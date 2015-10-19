#undef TRACE_SYSTEM
#define TRACE_SYSTEM owlfb-trace-events

#if !defined(_TRACE_OWLFB_DC_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_OWLFB_DC_H

#include <linux/tracepoint.h>

DECLARE_EVENT_CLASS(dc_in,
	    TP_PROTO(void * callbackarg),
	    TP_ARGS(callbackarg),

	    TP_STRUCT__entry(
		  __field(void * , callbackarg )
	    ),

	    TP_fast_assign(
		     __entry->callbackarg = callbackarg;
	    ),
	
	    TP_printk("dc_in callbackarg %p", __entry->callbackarg)
);

DEFINE_EVENT(dc_in, owlfb_dc_in,
	    TP_PROTO(void * callbackarg),
	    TP_ARGS(callbackarg)
);

DECLARE_EVENT_CLASS(dc_to_de,
	    TP_PROTO(void * callbackarg),
	    TP_ARGS(callbackarg),

	    TP_STRUCT__entry(
		  __field(void * , callbackarg )
	    ),

	    TP_fast_assign(
		     __entry->callbackarg = callbackarg;
	    ),
	
	    TP_printk("dc_in callbackarg %p", __entry->callbackarg)
);

DEFINE_EVENT(dc_to_de, owlfb_dc_to_de,
	    TP_PROTO(void * callbackarg),
	    TP_ARGS(callbackarg)
);

DECLARE_EVENT_CLASS(dc_out,
	    TP_PROTO(void * callbackarg),
	    TP_ARGS(callbackarg),

	    TP_STRUCT__entry(
		    __field(void * , callbackarg )
	    ),

	    TP_fast_assign(
		    __entry->callbackarg = callbackarg;
	    ),
		
		TP_printk("dc_out callbackarg =%p",
			  __entry->callbackarg)
);

DEFINE_EVENT(dc_out, owlfb_dc_out,
	    TP_PROTO(void * callbackarg),
	    TP_ARGS(callbackarg)
);



DECLARE_EVENT_CLASS(vsync,
	    TP_PROTO(int index, long long timestamp),
	    TP_ARGS(index,timestamp),

	    TP_STRUCT__entry(
		    __field(int  , index )
		    __field(long long , timestamp)
	    ),

	    TP_fast_assign(
		    __entry->index = index;
		    __entry->timestamp = timestamp;
	    ),
		
		TP_printk("vsync index %d timestamp %llu",__entry->index,
			  __entry->timestamp)
);

DEFINE_EVENT(vsync, owlfb_vsync,
	    TP_PROTO(int index, long long timestamp),
	    TP_ARGS(index,timestamp)
);


#endif /* _TRACE_STAND_HOTPLUG_H */

#include <trace/define_trace.h>
