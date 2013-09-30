#!/bin/sh

elapsed_time()
{
	SEC=$1
	(( SEC < 60 )) && echo -e "[Elapsed time: $SEC seconds]"
	(( SEC >60 && SEC < 3600 )) && echo -e "[Elapsed time: $(( SEC / 60 )) min $(( SEC % 60 )) seconds]"
	(( SEC > 3600 )) && echo -e "[Elapsed time: $(( SEC / 3600 )) hours $(( (SEC % 3600) / 60 )) minutes  $(( SEC % 60 )) seconds]"
}
elapsed_time 6000
