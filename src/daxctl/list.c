#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <util/json.h>
#include <util/filter.h>
#include <json-c/json.h>
#include <daxctl/libdaxctl.h>
#include <util/parse-options.h>
#include <ccan/array_size/array_size.h>

static struct {
	bool devs;
	bool regions;
	bool idle;
} list;

static struct {
	const char *dev;
	int region_id;
} param = {
	.region_id = -1,
};

static int did_fail;
static int jflag = JSON_C_TO_STRING_PRETTY;

#define fail(fmt, ...) \
do { \
	did_fail = 1; \
	fprintf(stderr, "daxctl-%s:%s:%d: " fmt, \
			VERSION, __func__, __LINE__, ##__VA_ARGS__); \
} while (0)

static int num_list_flags(void)
{
	return list.regions + list.devs;
}

int cmd_list(int argc, const char **argv, void *ctx)
{
	const struct option options[] = {
		OPT_INTEGER('r', "region", &param.region_id, "filter by region"),
		OPT_STRING('d', "dev", &param.dev, "dev-id",
				"filter by dax device instance name"),
		OPT_BOOLEAN('D', "devices", &list.devs, "include dax device info"),
		OPT_BOOLEAN('R', "regions", &list.regions, "include dax region info"),
		OPT_BOOLEAN('i', "idle", &list.idle, "include idle devices"),
		OPT_END(),
	};
	const char * const u[] = {
		"daxctl list [<options>]",
		NULL
	};
	struct json_object *jregions = NULL;
	struct json_object *jdevs = NULL;
	struct daxctl_region *region;
	int i;

        argc = parse_options(argc, argv, options, u, 0);
	for (i = 0; i < argc; i++)
		error("unknown parameter \"%s\"\n", argv[i]);

	if (argc)
		usage_with_options(u, options);

	if (num_list_flags() == 0) {
		list.regions = param.region_id >= 0;
		list.devs = !!param.dev;
	}

	if (num_list_flags() == 0)
		list.devs = true;

	daxctl_region_foreach(ctx, region) {
		struct json_object *jregion = NULL;

		if (param.region_id >= 0 && param.region_id
				!= daxctl_region_get_id(region))
			continue;

		if (list.regions) {
			if (!jregions) {
				jregions = json_object_new_array();
				if (!jregions) {
					fail("\n");
					continue;
				}
			}

			jregion = util_daxctl_region_to_json(region,
					list.devs, param.dev, list.idle);
			if (!jregion) {
				fail("\n");
				continue;
			}
			json_object_array_add(jregions, jregion);
		} else if (list.devs)
			jdevs = util_daxctl_devs_to_list(region,
					jdevs, param.dev, list.idle);
	}

	if (jregions)
		util_display_json_array(stdout, jregions, jflag);
	else if (jdevs)
		util_display_json_array(stdout, jdevs, jflag);

	if (did_fail)
		return -ENOMEM;
	return 0;
}
