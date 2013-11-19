#include <linux/module.h>
#include <linux/fs.h>
#include <linux/inet.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <net/protocol.h>
#include <net/sock.h>
#include <net/route.h>
#include <net/ip.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#define CSE536_MAJOR 234
#define IPPROTO_CSE536 234 // my protocol number

static int debug_enable = 0;
module_param(debug_enable, int, 0);
MODULE_PARM_DESC(debug_enable, "Enable module debug mode.");
struct file_operations cse536_fops;
typedef struct MessageHeader{
  uint32_t record_id;
  uint32_t final_clock;
  uint32_t orig_clock;
  __be32 source_ip;
   __be32 dest_ip;
}MessageHeader;

typedef struct Message
{
  MessageHeader header;
  uint8_t data[0xFF-sizeof(MessageHeader)];
}Message;


uint32_t clock;
struct semaphore sem;
struct semaphore mutex;
struct semaphore sem_buf;
struct semaphore waitmutex;
uint32_t wait;
struct cse536buffer
{
	struct cse536buffer *next;
	Message *data;
};

struct cse536buffer *cse536buffhead;

__be32 cse536_daddr, cse536_saddr;

static int cse536_sendmsg(char *data, size_t len);

void addData (Message *data)
{
	//down_interruptible(&sem_buf);
	struct cse536buffer  *tempread = cse536buffhead;
        printk ("cse536_addPendingRead adding pending read \n");
	if (cse536buffhead == NULL)
	{
		cse536buffhead = kmalloc (sizeof (struct cse536buffer), GFP_ATOMIC);
		cse536buffhead->data = kmalloc (sizeof (struct Message), GFP_ATOMIC);
		memcpy (cse536buffhead->data, data,256);
		cse536buffhead->next = NULL;
	}
	else
	{
		while (tempread->next != NULL)
		{
			tempread =  tempread->next;
		}
		tempread->next =  kmalloc (sizeof (struct cse536buffer), GFP_ATOMIC);
		tempread->next->data = kmalloc (sizeof (struct Message), GFP_ATOMIC);
		memcpy (tempread->next->data, data,256);
		
		tempread->next->next = NULL;
	}
        printk ("cse536_addPendingdataRead adding pending read Done \n");
	//up(&sem_buf);
}
int deleteData(void)
{
	//down_interruptible(&sem_buf);
        int result;
	struct cse536buffer * tempread ;
        printk ("cse536_Deletedataread : deleting pending read \n");

        if (cse536buffhead == NULL)
        {
          result = 1;
        }
        else
        {
           tempread = cse536buffhead->next;

           /* Free the data */
           kfree (cse536buffhead->data);
           cse536buffhead->data =NULL;

           /* free the top node */
           kfree (cse536buffhead);
           cse536buffhead = NULL;

           /* set next node as the top node */
           cse536buffhead = tempread;
           printk ("cse536_Deletedataread deleting pending read Done \n");

           result = 0;
        }
	//uo(&sem_buf);
    return result;
}



static int cse536_open(struct inode *inode, struct file *file)
{
	printk("cse536_open: successful\n");
	return 0;
}
static int cse536_release(struct inode *inode, struct file *file)
{
	printk("cse536_release: successful\n");
	return 0;
}
static ssize_t cse536_read(struct file *file, char *buf, size_t count,
loff_t *ptr)
{
	//struct cse536buffer *next;
	ssize_t retCount = count;
	if (count > 256)
		retCount = 256; // data buffer sizes standardized at 256, make sure not trying to read more
	down_interruptible(&sem_buf);
	if(cse536buffhead!=NULL){
		printk("cse536_read: returning %s bytes  \n", (cse536buffhead->data)->data);
		snprintf(buf, retCount, "ip:%pI4;lc:%u;oc:%u;Msg:%s",&((cse536buffhead->data->header).source_ip), (cse536buffhead->data->header).final_clock,((cse536buffhead->data->header).orig_clock), (cse536buffhead->data)->data);
		//memcpy(buf, cse536buffhead->data, retCount);
		deleteData();
		
	}
	up(&sem_buf);
	
	//retCount = sprintf(buf, "cse536");
	printk("cse536_read: returning %d bytes  \n", retCount);

	return retCount;
}
static ssize_t cse536_write(struct file *file, const char *buf,
size_t count, loff_t * ppos)
{	
	char *databuf, address[16],data[256];
	char *ipaddr;int timeout;
	Message* message1 = (Message*)kmalloc(sizeof(Message), GFP_ATOMIC); 
	
	ipaddr = strsep((char **)&buf,"\t");
        databuf = strsep((char **)&buf,"\n");
	memcpy(address, ipaddr, 16);
	cse536_daddr = in_aton(address);
	printk("cse536_write:ip and temp %s --- %s \n",ipaddr,databuf);
	printk("cse536 _write:\n");
	// initialize data memory to zeros
	memset(data, 0, 256);
	
	// copy write buffer to data array, data starts at 2nd byte in buffer
	memcpy(message1->data, databuf, 0xFF-sizeof(MessageHeader));
	message1->header.record_id =1;
	down_interruptible(&mutex);
	message1->header.final_clock=clock;
	message1->header.orig_clock=clock;
	up(&mutex);
	message1->header.source_ip=cse536_saddr;
	message1->header.dest_ip=cse536_daddr;
	memcpy(data,message1,256);
	
	printk("cse536_write - sending message first time: %s\n", data);
	cse536_sendmsg(data, 256);
	down_interruptible(&mutex);
	down_interruptible(&waitmutex);
	wait=clock;
	up(&waitmutex);
	up(&mutex);
	timeout = down_timeout(&sem, msecs_to_jiffies(5000));
	if(timeout)
	{
		
		cse536_sendmsg(data, 256);
		timeout = down_timeout(&sem, msecs_to_jiffies(5000));
		if(timeout)
		return -2;
	}
	down_interruptible(&mutex);
	clock=clock+1;
	up(&mutex);
	printk("cse536_write1: %s\n", message1->data);
	return -1;
}
// this method will send the message to the destination machine using ipv4
static int cse536_sendmsg(char *data, size_t len)
{
	struct sk_buff *skb;
	struct iphdr *iph;
	struct rtable *rt;
	struct net *net = &init_net;
	unsigned char *skbdata;
	
	// create and setup an sk_buff	
	skb = alloc_skb(sizeof(struct iphdr) + 4096, GFP_ATOMIC);
	skb_reserve(skb, sizeof(struct iphdr) + 1500);
	skbdata = skb_put(skb, len);
//	skb->csum = csum_and_copy_from_user(data, skbdata, len, 0, &err);
	memcpy(skbdata, data, len);

	// setup and add the ip header
	skb_push(skb, sizeof(struct iphdr));
	skb_reset_network_header(skb);
	iph = ip_hdr(skb);
	iph->version  = 4;
	iph->ihl      = 5;
	iph->tos      = 0;
	iph->frag_off = 0;
	iph->ttl      = 64;
	iph->daddr    = cse536_daddr;
	iph->saddr    = cse536_saddr;
	iph->protocol = IPPROTO_CSE536;	// my protocol number
	iph->id       = htons(1);
	iph->tot_len  = htons(skb->len);

	// get the route. this seems to be necessary, does not work without
	rt = ip_route_output(net, cse536_daddr, cse536_saddr, 0,0);	
	skb_dst_set(skb, &rt->dst);
	
	//printk("skb data: %s", skbdata);
	return ip_local_out(skb);
}

static long cse536_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	printk("cse536_ioctl: cmd=%d, arg=%ld\n", cmd, arg);
	return 0;
}
// called by ipv4 when a packet is received associated with my protocol number
int cse536_rcv(struct sk_buff *skb)
{	
	// setup a linked list item to add the data to the linked list buffer
	//struct cse536buffer *item = kmalloc(sizeof(struct cse536buffer), GFP_ATOMIC);
	char rcvd[256];int localwait=0;
	//down_interruptible(&mutex);
	Message *content;
	content = (Message*)kmalloc(sizeof(Message), GFP_ATOMIC); 
	memset(content, 0, 256); 
	memcpy(content, skb->data, skb->len);
	down_interruptible(&waitmutex); 
	localwait=wait;
	up(&waitmutex);
	if((content->header).record_id==1)
	{
		down_interruptible(&sem_buf);
		(content->header).final_clock=clock;
		addData(content);
		up(&sem_buf);
		printk("Inside the cse rcv record =1 \n");
		down_interruptible(&mutex);
		if((content->header).orig_clock < clock)
		{
			(content->header).final_clock = clock+1;
			clock=clock+1;
		}			
		else
		{
			(content->header).final_clock=(content->header).orig_clock+1;
						
			clock = (content->header).orig_clock +1;
			
		}
		printk("recieved : %s" ,content->data);
		(content->header).final_clock=clock;
		(content->header).record_id=0;
		up(&mutex);
		memcpy(rcvd,content,256);
		cse536_daddr = (content->header).source_ip;
		
		cse536_sendmsg(rcvd, 256);
				
	
	}

	else if(content->header.record_id==0 && localwait==content->header.orig_clock)
	{
		up(&sem); 
	}
	
	printk("Receive handler called. Record: %d bytes: %s clock: %d IPV4 \n", (content->header).record_id, content->data, content->header.orig_clock );

	return 0;
}
void cse536_err(struct sk_buff *skb, __u32 info)
{
	printk("Error handler called.\n");
}

/* Register with IP layer.  */
static const struct net_protocol cse536_protocol = {
	.handler     = cse536_rcv,
	.err_handler = cse536_err,
	.no_policy   = 1,
};

static int cse536_add_protocol(void)
{
	/* Register protocol with inet layer.  */
	if (inet_add_protocol(&cse536_protocol, IPPROTO_CSE536) < 0)
		return -EAGAIN;
	return 0;
}
static void cse536_del_protocol(void)
{
	inet_del_protocol(&cse536_protocol, IPPROTO_CSE536);
}
// get the local ip address so that it does not have to be set manually
static void getlocaladdress(void) 
{
	struct net_device *eth0 = dev_get_by_name(&init_net, "eth0");
	struct in_device *ineth0 = in_dev_get(eth0);

	for_primary_ifa(ineth0){
		cse536_saddr = ifa->ifa_address;
  	} endfor_ifa(ineth0);
}
static int __init cse536_init(void)
{
	int ret;
	
	printk("cse536 module Init - debug mode is %s\n",
	debug_enable ? "enabled" : "disabled");
	sema_init(&sem,0);
	sema_init(&mutex,1);
	sema_init(&sem_buf,1);
	sema_init(&waitmutex,1);
	getlocaladdress();
	printk("cse536 module Init - using local address: %pI4\n", &cse536_saddr);

	// initialize buffer
	//cse536buffhead = 0;
	//cse536bufftail = 0;

	// register my protocol
	ret = cse536_add_protocol();
	if ( ret < 0 ) {
		printk("Error registering cse536 protocol\n");
		goto cse536_fail1;
	}

	ret = register_chrdev(CSE536_MAJOR, "cse5361", &cse536_fops);
	if (ret < 0) {
		printk("Error registering cse536 device\n");
		goto cse536_fail1;
	}
	printk("cse536: registered module successfully!\n");
	
	/* Init processing here... */
	return 0;
	cse536_fail1:
	return ret;
}
static void __exit cse536_exit(void)
{
	cse536_del_protocol();
	unregister_chrdev(CSE536_MAJOR, "cse5361"); 
	printk("cse536 module Exit\n");
}



struct file_operations cse536_fops = {
	owner: THIS_MODULE,
	read: cse536_read,
	write: cse536_write,
	unlocked_ioctl: cse536_ioctl,
	open: cse536_open,
	release: cse536_release,
};
module_init(cse536_init);
module_exit(cse536_exit);

MODULE_AUTHOR("SatyaSwaroop Boddu");
MODULE_DESCRIPTION("cse536 Module");
MODULE_LICENSE("GPL");

