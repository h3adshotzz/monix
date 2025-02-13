//===----------------------------------------------------------------------===//
//
//                                  tinyOS
//                             The Monix Kernel
//
// 	This program is free software: you can redistribute it and/or modify
// 	it under the terms of the GNU General Public License as published by
// 	the Free Software Foundation, either version 3 of the License, or
// 	(at your option) any later version.
//
// 	This program is distributed in the hope that it will be useful,
// 	but WITHOUT ANY WARRANTY; without even the implied warranty of
// 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// 	GNU General Public License for more details.
//
// 	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//	Copyright (C) 2023-2025, Harry Moulton <me@h3adsh0tzz.com>
//
//===----------------------------------------------------------------------===//

#define pr_fmt(fmt)	"processor: " fmt

#include <kern/processor.h>
#include <kern/cpu.h>
#include <kern/defaults.h>
#include <kern/mm/zalloc.h>
#include <libkern/types.h>

#include <libkern/list.h>
#include <tinylibc/string.h>

//active, idle queues, counts, etc

/* processor information */
list_t		active_processors;
list_t		idle_processors;

integer_t	processor_count;
processor_t	*primary_processor;

integer_t	primary_cpu_id = 0;

zone_t		*processor_zone;

list_t		tasks	__ATTRIB_SECT_DATA;
list_t		threads	__ATTRIB_SECT_DATA;


/**
 * processor_init
 * 
 * Configure the processor interface, initialise the lists for tasks, threads,
 * and processors.
*/
void processor_init()
{
	INIT_LIST_HEAD(&active_processors);
	INIT_LIST_HEAD(&idle_processors);

	INIT_LIST_HEAD(&tasks);
	INIT_LIST_HEAD(&threads);

	/* create the processor struct zone */
	processor_zone = zone_create(sizeof(processor_t),
						CPU_NUMBER_MAX * sizeof(processor_t),
						"processor_zone");

	/* create the primary (boot) processor */
	primary_processor = processor_create(primary_cpu_id);
	cpu_set_processor(primary_processor->cpu_id, primary_processor);

	pr_info("processor_init complete\n");
}

/**
 * processor_create
 *
 * Create and initialise a processor struct with a given cpu_id.
*/
processor_t *processor_create(integer_t cpu_id)
{
	processor_t *processor;

	/* create the processor struct within the zone */
	processor = (processor_t *) zalloc(processor_zone);
	memset(processor, '\0', sizeof(processor_t));

	/* to start with, the processor is marked as "inactive" */
	processor->state = PROCESSOR_STATE_INACTIVE;

	processor->active_thread = processor->next_thread = 
		processor->idle_thread = THREAD_NULL;
	processor->priority = THREAD_PRIORITY_LOW;
	processor->cpu_id = cpu_id;

	list_add_tail(&processor->proc_list, &idle_processors);
	processor_count++;

	pr_info("created processor with cpu_id '%d': 0x%lx\n",
		cpu_id, processor);
	return processor;
}
