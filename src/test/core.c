#include <linux/version.h>
#include <sys/utsname.h>
#include <libkmod.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <test.h>

#include <util/log.h>
#include <util/sysfs.h>
#include <ccan/array_size/array_size.h>

#define KVER_STRLEN 20

struct ndctl_test {
	unsigned int kver;
	int attempt;
	int skip;
};

static unsigned int get_system_kver(void)
{
	const char *kver = getenv("KVER");
	struct utsname utsname;
	int a, b, c;

	if (!kver) {
		uname(&utsname);
		kver = utsname.release;
	}

	if (sscanf(kver, "%d.%d.%d", &a, &b, &c) != 3)
		return LINUX_VERSION_CODE;

	return KERNEL_VERSION(a,b,c);
}

struct ndctl_test *ndctl_test_new(unsigned int kver)
{
	struct ndctl_test *test = calloc(1, sizeof(*test));

	if (!test)
		return NULL;

	if (!kver)
		test->kver = get_system_kver();
	else
		test->kver = kver;

	return test;
}

int ndctl_test_result(struct ndctl_test *test, int rc)
{
	if (ndctl_test_get_skipped(test))
		fprintf(stderr, "attempted: %d skipped: %d\n",
				ndctl_test_get_attempted(test),
				ndctl_test_get_skipped(test));
	if (rc && rc != 77)
		return rc;
	if (ndctl_test_get_skipped(test) >= ndctl_test_get_attempted(test))
		return 77;
	/* return success if no failures and at least one test not skipped */
	return 0;
}

static char *kver_str(char *buf, unsigned int kver)
{
	snprintf(buf, KVER_STRLEN, "%d.%d.%d",  (kver >> 16) & 0xffff,
			(kver >> 8) & 0xff, kver & 0xff);
	return buf;
}

int __ndctl_test_attempt(struct ndctl_test *test, unsigned int kver,
		const char *caller, int line)
{
	char requires[KVER_STRLEN], current[KVER_STRLEN];

	test->attempt++;
	if (kver <= test->kver)
		return 1;
	fprintf(stderr, "%s: skip %s:%d requires: %s current: %s\n",
			__func__, caller, line, kver_str(requires, kver),
			kver_str(current, test->kver));
	test->skip++;
	return 0;
}

void __ndctl_test_skip(struct ndctl_test *test, const char *caller, int line)
{
	test->skip++;
	test->attempt = test->skip;
	fprintf(stderr, "%s: explicit skip %s:%d\n", __func__, caller, line);
}

int ndctl_test_get_attempted(struct ndctl_test *test)
{
	return test->attempt;
}

int ndctl_test_get_skipped(struct ndctl_test *test)
{
	return test->skip;
}

int nfit_test_init(struct kmod_ctx **ctx, struct kmod_module **mod,
		int log_level)
{
	int rc;
	unsigned int i;
	struct log_ctx log_ctx;
	const char *list[] = {
		"nfit",
		"dax",
		"dax_pmem",
		"libnvdimm",
		"nd_blk",
		"nd_btt",
		"nd_e820",
		"nd_pmem",
	};

	log_init(&log_ctx, "test/init", "NDCTL_TEST");
	log_ctx.log_priority = log_level;

	*ctx = kmod_new(NULL, NULL);
	if (!*ctx)
		return -ENXIO;
	kmod_set_log_priority(*ctx, log_level);

	/*
	 * Check that all nfit, libnvdimm, and device-dax modules are
	 * the mocked versions. If they are loaded, check that they have
	 * the "out-of-tree" kernel taint, otherwise check that they
	 * come from the "/lib/modules/<KVER>/extra" directory.
	 */
	for (i = 0; i < ARRAY_SIZE(list); i++) {
		char attr[SYSFS_ATTR_SIZE];
		const char *name = list[i];
		const char *path;
		char buf[100];
		int state;

		rc = kmod_module_new_from_name(*ctx, name, mod);
		if (rc) {
			log_err(&log_ctx, "%s.ko: missing\n", name);
			break;
		}

		path = kmod_module_get_path(*mod);
		if (!path) {
			log_err(&log_ctx, "%s.ko: failed to get path\n", name);
			break;
		}

		if (!strstr(path, "/extra/")) {
			log_err(&log_ctx, "%s.ko: appears to be production version: %s\n",
					name, path);
			break;
		}

		state = kmod_module_get_initstate(*mod);
		if (state == KMOD_MODULE_LIVE) {
			sprintf(buf, "/sys/module/%s/taint", name);
			rc = __sysfs_read_attr(&log_ctx, buf, attr);
			if (rc < 0) {
				log_err(&log_ctx, "%s.ko: failed to read %s\n",
						name, buf);
				break;
			}

			if (strcmp(attr, "O") != 0) {
				log_err(&log_ctx, "%s.ko: expected taint: O got: %s\n",
						name, attr);
				break;
			}
		} else if (state == KMOD_MODULE_BUILTIN) {
			log_err(&log_ctx, "%s: must be built as a module\n", name);
			break;
		}
	}

	if (i < ARRAY_SIZE(list)) {
		kmod_unref(*ctx);
		return -ENXIO;
	}

	rc = kmod_module_new_from_name(*ctx, "nfit_test", mod);
	if (rc < 0) {
		kmod_unref(*ctx);
		return rc;
	}

	rc = kmod_module_probe_insert_module(*mod, KMOD_PROBE_APPLY_BLACKLIST,
			NULL, NULL, NULL, NULL);
	if (rc)
		kmod_unref(*ctx);
	return rc;
}
