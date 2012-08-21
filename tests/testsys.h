#ifndef TESTSYS_H
#define TESTSYS_H

// hawkey
#include "src/packagelist.h"
#include "src/sack.h"

struct TestGlobals_s {
    char *repo_dir;
    HySack sack;
    char *tmpdir;
};

#define UNITTEST_DIR "/tmp/hawkeyXXXXXX"
#define YUM_DIR_SUFFIX "yum/repodata/"
#define YUM_REPO_NAME "nevermac"
#define TEST_FIXED_ARCH "x86_64"
#define TEST_META_SOLVABLES_COUNT 2
#define TEST_EXPECT_SYSTEM_PKGS 7
#define TEST_EXPECT_SYSTEM_NSOLVABLES \
    (TEST_META_SOLVABLES_COUNT + TEST_EXPECT_SYSTEM_PKGS)
#define TEST_EXPECT_MAIN_NSOLVABLES 14
#define TEST_EXPECT_UPDATES_NSOLVABLES 2
#define TEST_EXPECT_YUM_NSOLVABLES 2

/* global data used to pass values from fixtures to tests */
extern struct TestGlobals_s test_globals;

HyPackage by_name(HySack sack, const char *name);
void dump_packagelist(HyPackageList plist);
HyRepo glob_for_repofiles(Pool *pool, const char *repo_name, const char *path);
int load_repo(Pool *pool, const char *name, const char *path, int installed);
int logfile_size(HySack sack);
int query_count_results(HyQuery query);
HyRepo repo_by_name(Pool *pool, const char *name);

/* fixtures */
void setup_empty_sack(void);
void setup(void);
void setup_with_main(void);
void setup_with_updates(void);
void setup_all(void);
void setup_yum_sack(HySack sack, const char *yum_repo_name);
void setup_yum(void);
void teardown(void);

#endif /* TESTSYS_H */
