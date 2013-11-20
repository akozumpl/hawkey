/*
 * Copyright (C) 2013 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>

#include <check.h>
#include <string.h>

// hawkey
#include "src/nevra.h"
#include "src/subject.h"
#include "fixtures.h"
#include "testshared.h"
#include "test_suites.h"

const char inp_fof[] = "four-of-fish-8:3.6.9-11.fc100.x86_64";
const char inp_fof_noepoch[] = "four-of-fish-3.6.9-11.fc100.x86_64";
const char inp_fof_nev[] = "four-of-fish-8:3.6.9";
const char inp_fof_na[] = "four-of-fish-3.6.9.i686";

START_TEST(nevra1)
{
    HyNevra nevra = hy_nevra_create();

    ck_assert_int_eq(
        hy_nevra_possibility((char *) inp_fof, HY_FORM_NEVRA, nevra), 0);
    ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_NAME), "four-of-fish");
    ck_assert_int_eq(nevra->epoch, 8);
    ck_assert_str_eq(nevra->version, "3.6.9");
    ck_assert_str_eq(nevra->release, "11.fc100");
    ck_assert_str_eq(nevra->arch, "x86_64");

    hy_nevra_free(nevra);
}
END_TEST


START_TEST(nevra2)
{
    HyNevra nevra = hy_nevra_create();

    ck_assert_int_eq(
        hy_nevra_possibility((char *) inp_fof_noepoch, HY_FORM_NEVRA, nevra), 0);
    ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_NAME), "four-of-fish");
    ck_assert_int_eq(nevra->epoch, -1);
    ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_VERSION), "3.6.9");
    ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_RELEASE), "11.fc100");
    ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_ARCH), "x86_64");

    hy_nevra_free(nevra);
}
END_TEST

START_TEST(nevr)
{
    HyNevra nevra = hy_nevra_create();

    ck_assert_int_eq(
        hy_nevra_possibility((char *) inp_fof, HY_FORM_NEVR, nevra), 0);
    ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_NAME), "four-of-fish");
    ck_assert_int_eq(nevra->epoch, 8);
    ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_VERSION), "3.6.9");
    ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_RELEASE), "11.fc100.x86_64");
    fail_unless(nevra->arch == NULL);

    hy_nevra_free(nevra);
}
END_TEST

START_TEST(nevr_fail)
{
    HyNevra nevra = hy_nevra_create();

    ck_assert_int_eq(
        hy_nevra_possibility((char *) "four-of", HY_FORM_NEVR, nevra), -1);

    hy_nevra_free(nevra);
}
END_TEST

START_TEST(nev)
{
    HyNevra nevra = hy_nevra_create();

    ck_assert_int_eq(
        hy_nevra_possibility((char *) inp_fof_nev, HY_FORM_NEV, nevra), 0);
    ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_NAME), "four-of-fish");
    ck_assert_int_eq(nevra->epoch, 8);
    ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_VERSION), "3.6.9");
    fail_unless(nevra->release == NULL);
    fail_unless(nevra->arch == NULL);

    hy_nevra_free(nevra);
}
END_TEST

START_TEST(na)
{
    HyNevra nevra = hy_nevra_create();

    ck_assert_int_eq(
        hy_nevra_possibility((char *) inp_fof_na, HY_FORM_NA, nevra), 0);
    ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_NAME), "four-of-fish-3.6.9");
    ck_assert_int_eq(nevra->epoch, -1);
    fail_unless(nevra->version == NULL);
    ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_ARCH), "i686");

    hy_nevra_free(nevra);
}
END_TEST

START_TEST(combined1)
{
    HyNevra nevra;

    int count = 0;
    FOR_NEVRAS_POSSIBILITIES(inp_fof, nevra, forms_most_spec) {
        count++;
        if (count ==1) {
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_NAME), "four-of-fish");
            ck_assert_int_eq(nevra->epoch, 8);
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_VERSION), "3.6.9");
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_RELEASE), "11.fc100");
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_ARCH), "x86_64");
        } else if (count == 2) {
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_NAME), "four-of-fish");
            ck_assert_int_eq(nevra->epoch, 8);
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_VERSION), "3.6.9");
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_RELEASE), "11.fc100.x86_64");
            fail_unless((nevra->arch) == NULL);
        }
    }
    ck_assert_int_eq(count, 2);
}
END_TEST

START_TEST(combined2)
{
    HyNevra nevra;

    int count = 0;
    FOR_NEVRAS_POSSIBILITIES(inp_fof_noepoch, nevra, forms_most_spec) {
        count++;
        if (count ==1) {
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_NAME), "four-of-fish");
            ck_assert_int_eq(nevra->epoch, -1);
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_VERSION), "3.6.9");
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_RELEASE), "11.fc100");
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_ARCH), "x86_64");
        } else if (count == 2) {
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_NAME), "four-of-fish");
            ck_assert_int_eq(nevra->epoch, -1);
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_VERSION), "3.6.9");
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_RELEASE), "11.fc100.x86_64");
            fail_unless(nevra->arch == NULL);
        } else if (count == 3) {
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_NAME), "four-of-fish-3.6.9");
            ck_assert_int_eq(nevra->epoch, -1);
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_VERSION), "11.fc100.x86_64");
            fail_unless(nevra->release == NULL);
            fail_unless(nevra->arch == NULL);
        } else if (count == 4) {
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_NAME), "four-of-fish-3.6.9-11.fc100");
            ck_assert_int_eq(nevra->epoch, -1);
            fail_unless(nevra->version == NULL);
            fail_unless(nevra->release == NULL);
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_ARCH), "x86_64");
        } else if (count == 5) {
            ck_assert_str_eq(hy_nevra_get_string(nevra, HY_NEVRA_NAME), "four-of-fish-3.6.9-11.fc100.x86_64");
            ck_assert_int_eq(nevra->epoch, -1);
            fail_unless(nevra->version == NULL);
            fail_unless(nevra->release == NULL);
            fail_unless(nevra->arch == NULL);
        }
    }
    ck_assert_int_eq(count, 5);
}
END_TEST

Suite *
subject_suite(void)
{
    Suite *s = suite_create("Subject");
    TCase *tc = tcase_create("Core");
    tcase_add_test(tc, nevra1);
    tcase_add_test(tc, nevra2);
    tcase_add_test(tc, nevr);
    tcase_add_test(tc, nevr_fail);
    tcase_add_test(tc, nev);
    tcase_add_test(tc, na);
    tcase_add_test(tc, combined1);
    tcase_add_test(tc, combined2);
    suite_add_tcase(s, tc);

    return s;
}
