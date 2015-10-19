#undef TRACE_SYSTEM
#define TRACE_SYSTEM stand_hotplug

#if !defined(_TRACE_STAND_HOTPLUG_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_STAND_HOTPLUG_H

#include <linux/tracepoint.h>

DECLARE_EVENT_CLASS(plug_in,
	    TP_PROTO(unsigned long type, unsigned long hotpluging_rate, unsigned long avg_load,
		     unsigned long nr_online_cpu, unsigned long nr_running,
		     unsigned long cur_freq, unsigned long freq_min),
	    TP_ARGS(type, hotpluging_rate, avg_load, nr_online_cpu, nr_running, cur_freq, freq_min),

	    TP_STRUCT__entry(
		    __field(unsigned long, type    )
			__field(unsigned long, hotpluging_rate    )
		    __field(unsigned long, avg_load      )
		    __field(unsigned long, nr_online_cpu   )
		    __field(unsigned long, nr_running )
		    __field(unsigned long, cur_freq   )
			__field(unsigned long, freq_min   )
	    ),

	    TP_fast_assign(
		    __entry->type = type;
			__entry->hotpluging_rate = hotpluging_rate;
		    __entry->avg_load = avg_load;
		    __entry->nr_online_cpu = nr_online_cpu;
		    __entry->nr_running = nr_running;
		    __entry->cur_freq = cur_freq;
			__entry->freq_min = freq_min;
	    ),
	
	    TP_printk("type=%lu hotpluging_rate=%lu avg_load=%lu nr_online_cpu=%lu nr_running=%lu cur_freq=%lu freq_min=%lu",
		      __entry->type, __entry->hotpluging_rate, __entry->avg_load, __entry->nr_online_cpu,
		      __entry->nr_running, __entry->cur_freq, __entry->freq_min)
);

DEFINE_EVENT(plug_in, stand_hotplug_in,
	    TP_PROTO(unsigned long type, unsigned long hotpluging_rate, unsigned long avg_load,
		     unsigned long nr_online_cpu, unsigned long nr_running,
		     unsigned long cur_freq, unsigned long freq_min),
	    TP_ARGS(type, hotpluging_rate, avg_load, nr_online_cpu, nr_running, cur_freq, freq_min)
);

DECLARE_EVENT_CLASS(plug_out,
	    TP_PROTO(unsigned long type, unsigned long hotpluging_rate, unsigned long avg_load,
		     unsigned long nr_online_cpu, unsigned long down_count,
		     unsigned long load_min, unsigned long cur_freq,
			 unsigned long freq_min),
	    TP_ARGS(type, hotpluging_rate, avg_load, nr_online_cpu, down_count, load_min, cur_freq, freq_min),

	    TP_STRUCT__entry(
		    __field(unsigned long, type    )
			__field(unsigned long, hotpluging_rate    )
		    __field(unsigned long, avg_load      )
		    __field(unsigned long, nr_online_cpu   )
		    __field(unsigned long, down_count )
		    __field(unsigned long, load_min   )
			__field(unsigned long, cur_freq   )
			__field(unsigned long, freq_min   )
	    ),

	    TP_fast_assign(
		    __entry->type = type;
			__entry->hotpluging_rate = hotpluging_rate;
		    __entry->avg_load = avg_load;
		    __entry->nr_online_cpu = nr_online_cpu;
		    __entry->down_count = down_count;
		    __entry->load_min = load_min;
			__entry->cur_freq = cur_freq;
			__entry->freq_min = freq_min;
	    ),
		
		TP_printk("type=%lu hotpluging_rate=%lu avg_load=%lu nr_online_cpu=%lu down_count=%lu load_min=%lu cur_freq=%lu freq_min=%lu",
			  __entry->type, __entry->hotpluging_rate, __entry->avg_load, __entry->nr_online_cpu,
			  __entry->down_count, __entry->load_min, __entry->cur_freq, __entry->freq_min)
);

DEFINE_EVENT(plug_out, stand_hotplug_out,
	    TP_PROTO(unsigned long type, unsigned long hotpluging_rate, unsigned long avg_load,
		     unsigned long nr_online_cpu, unsigned long down_count,
		     unsigned long load_min, unsigned long cur_freq,
			 unsigned long freq_min),
	    TP_ARGS(type, hotpluging_rate, avg_load, nr_online_cpu, down_count, load_min, cur_freq, freq_min)
);


#endif /* _TRACE_STAND_HOTPLUG_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
