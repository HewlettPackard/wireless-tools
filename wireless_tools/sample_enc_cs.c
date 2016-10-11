/* Note : this particular snipset of code is available under
 * the LGPL, MPL or BSD license (at your choice).
 * Jean II
 */

#define MAX_KEY_SIZE	16
#define	MAX_KEYS	8
int	key_on = 0;
int	key_open = 1;
int	key_current = 0;
char	key_table[MAX_KEYS][MAX_KEY_SIZE];
int	key_size[MAX_KEYS];

#if WIRELESS_EXT > 8
     case SIOCSIWENCODE:
       /* Basic checking... */
       if(wrq->u.encoding.pointer != (caddr_t) 0)
	 {
	   int	index = (wrq->u.encoding.flags & IW_ENCODE_INDEX) - 1;

	   /* Check the size of the key */
	   if(wrq->u.encoding.length > MAX_KEY_SIZE)
	     {
	       ret = -EINVAL;
	       break;
	     }

	   /* Check the index */
	   if((index < 0) || (index >= MAX_KEYS))
	     index = key_current;

	   /* Copy the key in the driver */
	   if(copy_from_user(key_table[index], wrq->u.encoding.pointer,
			     wrq->u.encoding.length))
	     {
	       key_size[index] = 0;
	       ret = -EFAULT;
	       break;
	     }
	   key_size[index] = wrq->u.encoding.length;
	   key_on = 1;
	 }
       else
	 {
	   int	index = (wrq->u.encoding.flags & IW_ENCODE_INDEX) - 1;
	   /* Do we want to just set the current key ? */
	   if((index >= 0) && (index < MAX_KEYS))
	     {
	       if(key_size[index] > 0)
		 {
		   key_current = index;
		   key_on = 1;
		 }
	       else
		 ret = -EINVAL;
	     }
	 }

       /* Read the flags */
       if(wrq->u.encoding.flags & IW_ENCODE_DISABLED)
	 key_on = 0;	/* disable encryption */
       if(wrq->u.encoding.flags & IW_ENCODE_RESTRICTED)
	 key_open = 0;	/* disable open mode */
       if(wrq->u.encoding.flags & IW_ENCODE_OPEN)
	 key_open = 1;	/* enable open mode */

       break;

     case SIOCGIWENCODE:
       /* only super-user can see encryption key */
       if(!suser())
	 {
	   ret = -EPERM;
	   break;
	 }

       /* Basic checking... */
       if(wrq->u.encoding.pointer != (caddr_t) 0)
	 {
	   int	index = (wrq->u.encoding.flags & IW_ENCODE_INDEX) - 1;

	   /* Set the flags */
	   wrq->u.encoding.flags = 0;
	   if(key_on == 0)
	      wrq->u.encoding.flags |= IW_ENCODE_DISABLED;
	   if(key_open == 0)
	     wrq->u.encoding.flags |= IW_ENCODE_RESTRICTED;
	   else
	     wrq->u.encoding.flags |= IW_ENCODE_OPEN;

	   /* Which key do we want */
	   if((index < 0) || (index >= MAX_KEYS))
	     index = key_current;
	   wrq->u.encoding.flags |= index + 1;

	   /* Copy the key to the user buffer */
	   wrq->u.encoding.length = key_size[index];
	   if(copy_to_user(wrq->u.encoding.pointer, key_table[index],
			   key_size[index]))
	     ret = -EFAULT;
	}
       break;
#endif	/* WIRELESS_EXT > 8 */

#if WIRELESS_EXT > 8
	  range.encoding_size[0] = 8;	/* DES = 64 bits key */
	  range.encoding_size[1] = 16;
	  range.num_encoding_sizes = 2;
	  range.max_encoding_tokens = 8;
#endif /* WIRELESS_EXT > 8 */
