/*
 * Copyright (c) 2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <pm/policy.h>
#include <sys/time_units.h>
#include <sys_clock.h>
#include <ztest.h>

#ifdef CONFIG_PM_POLICY_DEFAULT
/**
 * @brief Test the behavior of pm_policy_next_state() when
 * CONFIG_PM_POLICY_DEFAULT=y.
 */
static void test_pm_policy_next_state_default(void)
{
	const struct pm_state_info *next;

	/* cpu 0 */
	next = pm_policy_next_state(0U, 0);
	zassert_equal(next, NULL, NULL);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(10999));
	zassert_equal(next, NULL, NULL);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE, NULL);
	zassert_equal(next->min_residency_us, 100000, NULL);
	zassert_equal(next->exit_latency_us, 10000, NULL);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(1099999));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE, NULL);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(1100000));
	zassert_equal(next->state, PM_STATE_SUSPEND_TO_RAM, NULL);
	zassert_equal(next->min_residency_us, 1000000, NULL);
	zassert_equal(next->exit_latency_us, 100000, NULL);

	next = pm_policy_next_state(0U, K_TICKS_FOREVER);
	zassert_equal(next->state, PM_STATE_SUSPEND_TO_RAM, NULL);

	/* cpu 1 */
	next = pm_policy_next_state(1U, 0);
	zassert_equal(next, NULL, NULL);

	next = pm_policy_next_state(1U, k_us_to_ticks_floor32(549999));
	zassert_equal(next, NULL, NULL);

	next = pm_policy_next_state(1U, k_us_to_ticks_floor32(550000));
	zassert_equal(next->state, PM_STATE_SUSPEND_TO_RAM, NULL);
	zassert_equal(next->min_residency_us, 500000, NULL);
	zassert_equal(next->exit_latency_us, 50000, NULL);

	next = pm_policy_next_state(1U, K_TICKS_FOREVER);
	zassert_equal(next->state, PM_STATE_SUSPEND_TO_RAM, NULL);
}

/**
 * @brief Test the behavior of pm_policy_next_state() when
 * states are allowed/disallowed and CONFIG_PM_POLICY_DEFAULT=y.
 */
static void test_pm_policy_next_state_default_allowed(void)
{
	bool active;
	const struct pm_state_info *next;

	/* initial state: PM_STATE_RUNTIME_IDLE allowed
	 * next state: PM_STATE_RUNTIME_IDLE
	 */
	active = pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE);
	zassert_false(active, NULL);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE, NULL);

	/* disallow PM_STATE_RUNTIME_IDLE
	 * next state: NULL (active)
	 */
	pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE);

	active = pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE);
	zassert_true(active, NULL);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_equal(next, NULL, NULL);

	/* allow PM_STATE_RUNTIME_IDLE again
	 * next state: PM_STATE_RUNTIME_IDLE
	 */
	pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE);

	active = pm_policy_state_lock_is_active(PM_STATE_RUNTIME_IDLE);
	zassert_false(active, NULL);

	next = pm_policy_next_state(0U, k_us_to_ticks_floor32(110000));
	zassert_equal(next->state, PM_STATE_RUNTIME_IDLE, NULL);
}
#else
static void test_pm_policy_next_state_default(void)
{
	ztest_test_skip();
}

static void test_pm_policy_next_state_default_allowed(void)
{
	ztest_test_skip();
}
#endif /* CONFIG_PM_POLICY_DEFAULT */

#ifdef CONFIG_PM_POLICY_CUSTOM
const struct pm_state_info *pm_policy_next_state(uint8_t cpu, int32_t ticks)
{
	static const struct pm_state_info state = {.state = PM_STATE_SOFT_OFF};

	ARG_UNUSED(cpu);
	ARG_UNUSED(ticks);

	return &state;
}

/**
 * @brief Test that a custom policy can be implemented when
 * CONFIG_PM_POLICY_CUSTOM=y.
 */
static void test_pm_policy_next_state_custom(void)
{
	const struct pm_state_info *next;

	next = pm_policy_next_state(0U, 0);
	zassert_equal(next->state, PM_STATE_SOFT_OFF, NULL);
}
#else
static void test_pm_policy_next_state_custom(void)
{
	ztest_test_skip();
}
#endif /* CONFIG_PM_POLICY_CUSTOM */

void test_main(void)
{
	ztest_test_suite(policy_api,
			 ztest_unit_test(test_pm_policy_next_state_default),
			 ztest_unit_test(test_pm_policy_next_state_default_allowed),
			 ztest_unit_test(test_pm_policy_next_state_custom));
	ztest_run_test_suite(policy_api);
}
