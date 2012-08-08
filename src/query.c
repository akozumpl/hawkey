#include <assert.h>

// libsolv
#include "solv/bitmap.h"
#include "solv/evr.h"
#include "solv/repo.h"
#include "solv/solver.h"
#include "solv/util.h"

// hawkey
#include "iutil.h"
#include "query_internal.h"
#include "package_internal.h"
#include "sack_internal.h"

struct _HyQuery {
    HySack sack;
    struct _Filter *filters;
    int nfilters;
    int downgrades; /* 1 for "only downgrades for installed packages" */
    int updates; /* 1 for "only updates for installed packages" */
    int latest; /* 1 for "only the latest version per arch" */
    int obsoleting; /* 1 for "only those obsoleting installed packages" */
};

struct _Filter {
    int filter_type;
    int keyname;
    char **matches;
    int nmatches;
    char *evr;
};

#define BLOCK_SIZE 15

static int
keyname2id(int keyname)
{
    switch(keyname) {
    case HY_PKG_NAME:
	return SOLVABLE_NAME;
    case HY_PKG_ARCH:
	return SOLVABLE_ARCH;
    case HY_PKG_EVR:
	return SOLVABLE_EVR;
    case HY_PKG_SUMMARY:
	return SOLVABLE_SUMMARY;
    case HY_PKG_FILE:
	return SOLVABLE_FILELIST;
    default:
	assert(0);
    }
}

static int
type2flags(int type, int keyname)
{
    int ret = 0;
    if (keyname == HY_PKG_FILE)
	ret |= SEARCH_FILES | SEARCH_COMPLETE_FILELIST;
    if (type & HY_ICASE)
	ret |= SEARCH_NOCASE;

    type &= ~HY_COMPARISON_FLAG_MASK;
    switch (type) {
    case HY_EQ:
	return ret | SEARCH_STRING;
    case HY_SUBSTR:
	return ret | SEARCH_SUBSTRING;
    case HY_GLOB:
	return ret | SEARCH_GLOB;
    default:
	assert(0); // not implemented
    }
}

static int
type2relflags(int type)
{
    int flags = 0;
    if (type & HY_EQ)
	flags |= REL_EQ;
    if (type & HY_LT)
	flags |= REL_LT;
    if (type & HY_GT)
	flags |= REL_GT;
    assert(flags);
    return flags;
}

static struct _Filter *
query_add_filter(HyQuery q, int nmatches)
{
    struct _Filter filter = {
	.matches = solv_calloc(nmatches, sizeof(char *)),
	.nmatches = nmatches
    };
    q->filters = solv_extend(q->filters, q->nfilters, 1, sizeof(filter),
			     BLOCK_SIZE);
    q->filters[q->nfilters] = filter; /* structure assignment */
    return q->filters + q->nfilters++;
}

static void
filter_dataiterator(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = sack_pool(q->sack);
    Dataiterator di;
    Id keyname = keyname2id(f->keyname);
    int flags = type2flags(f->filter_type, f->keyname);

    /* do an OR over all matches: */
    for (int i = 0; i < f->nmatches; ++i) {
	dataiterator_init(&di, pool, 0, 0,
			  keyname,
			  f->matches[i],
			  flags);
	while (dataiterator_step(&di))
	    MAPSET(m, di.solvid);
	dataiterator_free(&di);
    }
}

static void
filter_providers(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = sack_pool(q->sack);
    Id id_n, id_evr, r, p, pp;
    int flags;

    assert(f->nmatches == 1);

    sack_make_provides_ready(q->sack);
    id_n = pool_str2id(pool, f->matches[0], 0);
    id_evr = pool_str2id(pool, f->evr, 1);
    flags = type2relflags(f->filter_type);
    r = pool_rel2id(pool, id_n, id_evr, flags, 1);
    FOR_JOB_SELECT(p, pp, SOLVER_SOLVABLE_PROVIDES, r)
	MAPSET(m, p);
}

static void
filter_requires(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = sack_pool(q->sack);
    Queue provided;
    const char *matches[f->nmatches+1];
    HyQuery local_q = hy_query_create(q->sack);

    queue_init(&provided);

    if ((f->filter_type & ~HY_COMPARISON_FLAG_MASK) != HY_EQ)
        assert(0);

    // 0 ended list of matches
    for (int i = 0; i < f->nmatches; i++) matches[i] = f->matches[i];
    matches[f->nmatches] = NULL;

    hy_query_filter_in(local_q, HY_PKG_NAME, HY_EQ, matches);
    HyPackageList plist = hy_query_run(local_q);

    // Build list of provided stuff
    int count = hy_packagelist_count(plist);
    for (int i = 0; i < count; i++) {
        HyPackage pkg = hy_packagelist_get(plist, i);
        Solvable *s = pool_id2solvable(pkg->pool, pkg->id);
        Queue local_provided;
        queue_init(&local_provided);

        if (!solvable_lookup_idarray(s, SOLVABLE_PROVIDES, &local_provided)) {
            queue_free(&local_provided);
            continue;
        }

        for(int x = 0; local_provided.count; x++)
            queue_pushunique(&provided, queue_pop(&local_provided));
        queue_free(&local_provided);
    }

    // Find all packages requiring at least one item from provided queue
    for (Id solvable_id = 0; solvable_id < pool->nsolvables; ++solvable_id) {
        Solvable *s = pool_id2solvable(pool, solvable_id);
        Queue requires;
        int found = 0;

        queue_init(&requires);
        if (!solvable_lookup_idarray(s, SOLVABLE_REQUIRES, &requires)) {
            queue_free(&requires);
            continue;
        }

        for (int x = 0; requires.count; x++) {
            Id req_id = queue_pop(&requires);

            for (int y = 0; y < provided.count; y++) {
                Id prov_id = provided.elements[y];
                int cmp_ret = pool_match_dep(pool, prov_id, req_id);

                if (cmp_ret) {
                    found = 1;
                    break;
                }
            }
            if (found) break;
        }
        queue_free(&requires);
        if (found) MAPSET(m, solvable_id);
    }

    queue_free(&provided);
    hy_packagelist_free(plist);
    hy_query_free(local_q);
}

static void
filter_repo(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = sack_pool(q->sack);
    int i;
    Solvable *s;
    Repo *r;
    Id id, ourids[pool->nrepos];

    for (id = 0; id < pool->nrepos; ++id)
	ourids[id] = 0;
    FOR_REPOS(id, r) {
	for (i = 0; i < f->nmatches; i++) {
	    if (!strcmp(r->name, f->matches[i])) {
		ourids[id] = 1;
		break;
	    }
	}
    }

    for (i = 1; i < pool->nsolvables; ++i) {
	s = pool_id2solvable(pool, i);
	switch (f->filter_type & ~HY_COMPARISON_FLAG_MASK) {
	case HY_EQ:
	    if (s->repo && ourids[s->repo->repoid])
		MAPSET(m, i);
	    break;
	default:
	    assert(0);
	}
    }
}

static void
filter_updown(HyQuery q, int downgrade, Map *res)
{
    HySack sack = q->sack;
    Pool *pool = sack_pool(sack);
    int i;
    Map m;

    assert(pool->installed);
    sack_make_provides_ready(q->sack);
    map_init(&m, pool->nsolvables);
    for (i = 1; i < pool->nsolvables; ++i) {
	if (!MAPTST(res, i))
	    continue;
	Solvable *s = pool_id2solvable(pool, i);
	if (s->repo == pool->installed)
	    continue;
	if (downgrade && what_downgrades(pool, i) > 0)
	    MAPSET(&m, i);
	else if (!downgrade && what_upgrades(pool, i) > 0)
	    MAPSET(&m, i);
    }

    map_and(res, &m);
    map_free(&m);
}

static void
filter_latest(HyQuery q, Map *res)
{
    Pool *pool = sack_pool(q->sack);
    Queue samename;
    Id i, j, p, hp;
    Solvable *highest, *considered;
    int cmp;

    queue_init(&samename);
    for (i = 1; i < pool->nsolvables; ++i) {
	if (!MAPTST(res, i))
	    continue;

	hp = i;
	highest = pool_id2solvable(pool, hp);
	queue_empty(&samename);
	sack_same_names(q->sack, highest->name, highest->arch, &samename);
	/* now find the highest versioned package of those selected */
	for (j = 0; j < samename.count; ++j) {
	    p = samename.elements[j];
	    if (!MAPTST(res, p) || i == p)
		continue; /* out already */
	    considered = pool_id2solvable(pool, p);
	    cmp = pool_evrcmp(pool, highest->evr, considered->evr, EVRCMP_COMPARE);
	    if (cmp < 0) { /* new highest found */
		MAPCLR(res, hp);
		hp = p;
		highest = considered;
	    } else {
		/* note this is taken also for the same version case */
		MAPCLR(res, p);
	    }
	}
    }
    queue_free(&samename);
}

static void
filter_obsoleting(HyQuery q, Map *res)
{
    Pool *pool = sack_pool(q->sack);
    int obsprovides = pool_get_flag(pool, POOL_FLAG_OBSOLETEUSESPROVIDES);
    Id p, *pp;
    Solvable *s, *so;
    Map obsoleting;

    assert(pool->installed);
    map_init(&obsoleting, pool->nsolvables);
    sack_make_provides_ready(q->sack);
    for (p = 1; p < pool->nsolvables; ++p) {
	if (!MAPTST(res, p))
	    continue;
	s = pool_id2solvable(pool, p);
	if (!s->repo)
	    continue;
	for (pp = s->repo->idarraydata + s->obsoletes; *pp; ++pp) {
	    Id r, rr;

	    FOR_PROVIDES(r, rr, *pp) {
		assert(r != SYSTEMSOLVABLE);
		so = pool_id2solvable(pool, r);
		if (!obsprovides && !pool_match_nevr(pool, so, *pp))
		    continue; /* only matching pkg names */
		if (so->repo != pool->installed)
		    continue;
		MAPSET(&obsoleting, p);
		break;
	    }
	}
    }
    map_and(res, &obsoleting);
    map_free(&obsoleting);
}

HyQuery
hy_query_create(HySack sack)
{
    HyQuery q = solv_calloc(1, sizeof(*q));
    q->sack = sack;
    return q;
}

void
hy_query_free(HyQuery q)
{
    hy_query_clear(q);
    solv_free(q);
}

void
hy_query_clear(HyQuery q)
{
    for (int i = 0; i < q->nfilters; ++i) {
	struct _Filter *filterp = q->filters + i;
	for (int m = 0; m < filterp->nmatches; ++m)
	    solv_free(filterp->matches[m]);
	filterp->nmatches = 0;
	solv_free(filterp->matches);
	solv_free(filterp->evr);
    }
    solv_free(q->filters);
    q->filters = NULL;
    q->nfilters = 0;
}

void
hy_query_filter(HyQuery q, int keyname, int filter_type, const char *match)
{
    struct _Filter *filterp = query_add_filter(q, 1);
    filterp->filter_type = filter_type;
    filterp->keyname = keyname;
    filterp->matches[0] = solv_strdup(match);
}

void
hy_query_filter_in(HyQuery q, int keyname, int filter_type,
		   const char **matches)
{
    const unsigned count = count_nullt_array(matches);

    struct _Filter *filterp = query_add_filter(q, count);
    filterp->filter_type = filter_type;
    filterp->keyname = keyname;
    for (int i = 0; i < count; ++i)
	filterp->matches[i] = solv_strdup(matches[i]);
}

void
hy_query_filter_provides(HyQuery q, int filter_type, const char *name, const char *evr)
{
    struct _Filter *filterp = query_add_filter(q, 1);
    filterp->filter_type = filter_type;
    filterp->keyname = HY_PKG_PROVIDES;
    filterp->evr = solv_strdup(evr);
    filterp->matches[0] = solv_strdup(name);
}

/**
 * Narrows to only packages updating installed packages.
 *
 * This requires resolving and so makes the final query expensive.
 */
void
hy_query_filter_downgrades(HyQuery q, int val)
{
    q->downgrades = val;
}

/**
 * Narrows to only packages updating installed packages.
 *
 * This requires resolving and so makes the final query expensive.
 */
void
hy_query_filter_upgrades(HyQuery q, int val)
{
    q->updates = val;
}

/**
 * Narrows to only the highest version of a package per arch.
 */
void
hy_query_filter_latest(HyQuery q, int val)
{
    q->latest = val;
}

/**
 * Narrows to only the packages obsoleting installed packages.
 *
 */
void
hy_query_filter_obsoleting(HyQuery q, int val)
{
    q->obsoleting = val;
}

HyPackageList
hy_query_run(HyQuery q)
{
    Pool *pool = sack_pool(q->sack);
    HyPackageList plist;
    Map res, m;
    int i;

    map_init(&m, pool->nsolvables);
    map_init(&res, pool->nsolvables);
    map_setall(&res);
    MAPCLR(&res, SYSTEMSOLVABLE);
    for (i = 0; i < q->nfilters; ++i) {
	struct _Filter *f = q->filters + i;

	map_empty(&m);
	if (f->keyname == HY_PKG_PROVIDES) {
	    filter_providers(q, f, &m);
    } else if (f->keyname == HY_PKG_REQUIRES) {
        filter_requires(q, f, &m);
	} else if (f->keyname == HY_PKG_REPO) {
	    filter_repo(q, f, &m);
	} else {
	    filter_dataiterator(q, f, &m);
	}
	if (f->filter_type & HY_NOT)
	    map_subtract(&res, &m);
	else
	    map_and(&res, &m);
    }
    if (q->downgrades)
	filter_updown(q, 1, &res);
    if (q->updates)
	filter_updown(q, 0, &res);
    if (q->latest)
	filter_latest(q, &res);
    if (q->obsoleting)
	filter_obsoleting(q, &res);
    plist = hy_packagelist_create();
    for (i = 1; i < pool->nsolvables; ++i)
	if (MAPTST(&res, i))
	    hy_packagelist_push(plist, package_create(pool, i));
    map_free(&m);
    map_free(&res);

    return plist;
}

// internal
static int
filter_arch2job(const HyQuery q, const struct _Filter *f, Queue *job)
{
    if (f->filter_type != HY_EQ)
	return HY_E_QUERY;
    if (f->nmatches != 1)
	return HY_E_QUERY;
    if (job->count < 2)
	return HY_E_QUERY; // only apply arch if there's a name

    Pool *pool = sack_pool(q->sack);
    const char *arch = f->matches[0];
    Id archid = str2archid(pool, arch);

    if (archid == 0)
	return HY_E_QUERY;
    for (int i = 0; i < job->count; i += 2) {
	Id dep;
	assert(job->elements[i] == SOLVER_SOLVABLE_NAME);
	dep = pool_rel2id(pool, job->elements[i + 1],
			  archid, REL_ARCH, 1);
	job->elements[i] |= SOLVER_SETARCH;
	job->elements[i + 1] = dep;
    }
    return 0;
}

static int
filter_name2job(const HyQuery q, const struct _Filter *f, Queue *job)
{
    Pool *pool = sack_pool(q->sack);

    if (f->filter_type != HY_EQ)
	return HY_E_QUERY;

    sack_make_provides_ready(q->sack);
    for (int i = 0; i < f->nmatches; ++i) {
	const char *name = f->matches[i];
	Id id = pool_str2id(pool, name, 0);

	if (id)
	    queue_push2(job, SOLVER_SOLVABLE_NAME, id);
    }
    return 0;
}

int
query2job(const HyQuery q, Queue *job, int solver_action)
{
    int ret = 0;
    Queue job_query;

    queue_init(&job_query);
    for (int i = 0; i < q->nfilters; ++i) {
	const struct _Filter *f = q->filters + i;
	switch (f->keyname) {
	case HY_PKG_NAME:
	    ret = filter_name2job(q, f, &job_query);
	    break;
	case HY_PKG_ARCH:
	    break; // handled later
	default:
	    ret = HY_E_QUERY;
	    break;
	}
	if (ret)
	    goto finish;
    }

    /* handle archs */
    for (int i =0; i < q->nfilters; ++i) {
	const struct _Filter *f = q->filters + i;
	if (f->keyname != HY_PKG_ARCH)
	    continue;
	ret = filter_arch2job(q, f, &job_query);
	if (ret)
	    break;
    }

 finish:
    if (ret) {
	queue_free(&job_query);
	return ret;
    }

    for (int i = 0; i < job_query.count; i += 2)
	queue_push2(job,
		    job_query.elements[i] | solver_action,
		    job_query.elements[i + 1]);
    queue_free(&job_query);

    return 0;
}
