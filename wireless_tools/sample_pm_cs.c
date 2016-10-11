/* Note : this particular snipset of code is available under
 * the LGPL, MPL or BSD license (at your choice).
 * Jean II
 */

/* Backward compatibility for Wireless Extension 9 */
#ifndef IW_POWER_MODIFIER
#define IW_POWER_MODIFIER	0x000F	/* Modify a parameter */
#define IW_POWER_MIN		0x0001	/* Value is a minimum  */
#define IW_POWER_MAX		0x0002	/* Value is a maximum */
#define IW_POWER_RELATIVE	0x0004	/* Value is not in seconds/ms/us */
#endif IW_POWER_MODIFIER

struct net_local {
  int		pm_on;		// Power Management enabled
  int		pm_multi;	// Receive multicasts
  int		pm_period;	// Power Management period
  int		pm_period_auto;	// Power Management auto mode
  int		pm_max_period;	// Power Management max period
  int		pm_min_period;	// Power Management min period
  int		pm_timeout;	// Power Management timeout
};

      /* Set the desired Power Management mode */
    case SIOCSIWPOWER:
      /* Disable it ? */
      if(wrq->u.power.disabled)
	{
	  local->pm_on = 0;
	  local->need_commit = 1;
	}
      else
	{
	  /* Check mode */
	  switch(wrq->u.power.flags & IW_POWER_MODE)
	    {
	    case IW_POWER_UNICAST_R:
	      local->pm_multi = 0;
	      local->need_commit = 1;
	      break;
	    case IW_POWER_ALL_R:
	      local->pm_multi = 1;
	      local->need_commit = 1;
	      break;
	    case IW_POWER_ON:	/* None = ok */
	      break;
	    default:	/* Invalid */
	      rc = -EINVAL;
	    }
	  /* Set period */
	  if(wrq->u.power.flags & IW_POWER_PERIOD)
	    {
	      int	period = wrq->u.power.value/1000000;
	      /* Hum: check if within bounds... */

	      /* Activate PM */
	      local->pm_on = 1;
	      local->need_commit = 1;

	      /* Check min value */
	      if(wrq->u.power.flags & IW_POWER_MIN)
		{
		  local->pm_min_period = period;
		  local->pm_period_auto = 1;
		}
	      else
		/* Check max value */
		if(wrq->u.power.flags & IW_POWER_MAX)
		  {
		    local->pm_max_period = period;
		    local->pm_period_auto = 1;
		  }
		else
		  {
		    /* Fixed value */
		    local->pm_period = period;
		    local->pm_period_auto = 0;
		  }
	    }
	  /* Set timeout */
	  if(wrq->u.power.flags & IW_POWER_TIMEOUT)
	    {
	      /* Activate PM */
	      local->pm_on = 1;
	      local->need_commit = 1;
	      /* Fixed value in ms */
	      local->pm_timeout = wrq->u.power.value/1000;
	    }
	}
      break;

      /* Get the power management settings */
    case SIOCGIWPOWER:
      wrq->u.power.disabled = !local->pm_on;
      /* By default, display the period */
      if(!(wrq->u.power.flags & IW_POWER_TIMEOUT))
	{
	  int	inc_flags = wrq->u.power.flags;
	  wrq->u.power.flags = IW_POWER_PERIOD | IW_POWER_RELATIVE;
	  /* Check if auto */
	  if(local->pm_period_auto)
	    {
	      /* By default, the min */
	      if(!(inc_flags & IW_POWER_MAX))
		{
		  wrq->u.power.value = local->pm_min_period * 1000000;
		  wrq->u.power.flags |= IW_POWER_MIN;
		}
	      else
		{
		  wrq->u.power.value = local->pm_max_period * 1000000;
		  wrq->u.power.flags |= IW_POWER_MAX;
		}
	    }
	  else
	    {
	      /* Fixed value. Check the flags */
	      if(inc_flags & (IW_POWER_MIN | IW_POWER_MAX))
		rc = -EINVAL;
	      else
		wrq->u.power.value = local->pm_period * 1000000;
	    }
	}
      else
	{
	  /* Deal with the timeout - always fixed */
	  wrq->u.power.flags = IW_POWER_TIMEOUT;
	  wrq->u.power.value = local->pm_timeout * 1000;
	}
      if(local->pm_multi)
	wrq->u.power.flags |= IW_POWER_ALL_R;
      else
	wrq->u.power.flags |= IW_POWER_UNICAST_R;
      break;
#endif	/* WIRELESS_EXT > 8 */

#if WIRELESS_EXT > 9
      range.min_pmp = 1000000;	/* 1 units */
      range.max_pmp = 12000000;	/* 12 units */
      range.min_pmt = 1000;	/* 1 ms */
      range.max_pmt = 1000000;	/* 1 s */
      range.pmp_flags = IW_POWER_PERIOD | IW_POWER_RELATIVE |
        IW_POWER_MIN | IW_POWER_MAX;
      range.pmt_flags = IW_POWER_TIMEOUT;
      range.pm_capa = IW_POWER_PERIOD | IW_POWER_TIMEOUT | IW_POWER_UNICAST_R;
#endif /* WIRELESS_EXT > 9 */
