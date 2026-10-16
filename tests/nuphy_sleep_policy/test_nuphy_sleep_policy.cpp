/* Copyright 2026 NuPhy contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 */

#include "test_common.hpp"

extern "C" {
#include "keyboards/nuphy/common/power/sleep_policy.h"
}

namespace {

nuphy_sleep_policy_state_t default_state() {
    nuphy_sleep_policy_state_t policy_state{};
    policy_state.sleep_toggle       = true;
    policy_state.usb_sleep_toggle   = true;
    policy_state.deep_sleep_toggle  = true;
    policy_state.transport          = NUPHY_SLEEP_TRANSPORT_USB;
    policy_state.sleep_timeout      = 100;
    policy_state.rf_connected       = true;
    return policy_state;
}

nuphy_sleep_policy_result_t run_policy(const nuphy_sleep_policy_state_t& policy_state) {
    nuphy_sleep_policy_result_t result{};
    nuphy_sleep_policy_step(&policy_state, &result);
    return result;
}

} // namespace

TEST(NuphySleepPolicy, UsbSuspendRequestsSleepOnTwentiethCheck) {
    auto policy_state            = default_state();
    policy_state.usb_suspended   = true;

    for (uint8_t check = 1; check <= 19; ++check) {
        const auto result = run_policy(policy_state);

        EXPECT_EQ(result.action, NUPHY_SLEEP_ACTION_NONE);
        EXPECT_FALSE(result.goto_sleep);
        EXPECT_EQ(result.usb_suspend_debounce, check);
        policy_state.usb_suspend_debounce = result.usb_suspend_debounce;
    }

    const auto result = run_policy(policy_state);

    EXPECT_EQ(result.action, NUPHY_SLEEP_ACTION_SLEEP_REQUEST);
    EXPECT_TRUE(result.goto_sleep);
    EXPECT_EQ(result.usb_suspend_debounce, 20);
}

TEST(NuphySleepPolicy, UsbInactivityRequestsSleepWhenEnabled) {
    auto policy_state          = default_state();
    policy_state.inactivity_time = policy_state.sleep_timeout;

    const auto result = run_policy(policy_state);

    EXPECT_EQ(result.action, NUPHY_SLEEP_ACTION_SLEEP_REQUEST);
    EXPECT_TRUE(result.goto_sleep);
}

TEST(NuphySleepPolicy, UsbInactivityDoesNotRequestSleepWhenUsbSleepDisabled) {
    auto policy_state             = default_state();
    policy_state.usb_sleep_toggle = false;
    policy_state.usb_suspended    = false;
    policy_state.inactivity_time  = policy_state.sleep_timeout;

    const auto result = run_policy(policy_state);

    EXPECT_EQ(result.action, NUPHY_SLEEP_ACTION_NONE);
    EXPECT_FALSE(result.goto_sleep);
    EXPECT_EQ(result.inactivity_time, policy_state.inactivity_time);
}

TEST(NuphySleepPolicy, ChargingRequestsLightSleep) {
    auto policy_state          = default_state();
    policy_state.transport     = NUPHY_SLEEP_TRANSPORT_WIRELESS;
    policy_state.charging       = true;
    policy_state.goto_sleep    = true;

    const auto result = run_policy(policy_state);

    EXPECT_EQ(result.action, NUPHY_SLEEP_ACTION_LIGHT_SLEEP);
    EXPECT_FALSE(result.goto_sleep);
    EXPECT_TRUE(result.wakeup_prepare);
}

TEST(NuphySleepPolicy, WirelessSleepUsesDeepSleepWhenEnabled) {
    auto policy_state          = default_state();
    policy_state.transport     = NUPHY_SLEEP_TRANSPORT_WIRELESS;
    policy_state.charging       = false;
    policy_state.goto_sleep    = true;

    const auto result = run_policy(policy_state);

    EXPECT_EQ(result.action, NUPHY_SLEEP_ACTION_DEEP_SLEEP);
    EXPECT_FALSE(result.goto_sleep);
    EXPECT_FALSE(result.wakeup_prepare);
}

TEST(NuphySleepPolicy, WirelessSleepUsesLightSleepWhenDeepSleepDisabled) {
    auto policy_state          = default_state();
    policy_state.transport     = NUPHY_SLEEP_TRANSPORT_WIRELESS;
    policy_state.deep_sleep_toggle = false;
    policy_state.goto_sleep    = true;

    const auto result = run_policy(policy_state);

    EXPECT_EQ(result.action, NUPHY_SLEEP_ACTION_LIGHT_SLEEP);
    EXPECT_FALSE(result.goto_sleep);
    EXPECT_TRUE(result.wakeup_prepare);
}

TEST(NuphySleepPolicy, WirelessInactivityRequestsSleep) {
    auto policy_state              = default_state();
    policy_state.transport         = NUPHY_SLEEP_TRANSPORT_WIRELESS;
    policy_state.inactivity_time   = policy_state.sleep_timeout;

    const auto result = run_policy(policy_state);

    EXPECT_EQ(result.action, NUPHY_SLEEP_ACTION_SLEEP_REQUEST);
    EXPECT_TRUE(result.goto_sleep);
}

TEST(NuphySleepPolicy, WirelessLinkingTimeoutRequestsSleep) {
    auto policy_state              = default_state();
    policy_state.transport         = NUPHY_SLEEP_TRANSPORT_WIRELESS;
    policy_state.inactivity_time   = 0;
    policy_state.rf_linking_time   = NUPHY_SLEEP_POLICY_RF_LINKING_TIMEOUT;

    const auto result = run_policy(policy_state);

    EXPECT_EQ(result.action, NUPHY_SLEEP_ACTION_SLEEP_REQUEST);
    EXPECT_TRUE(result.goto_sleep);
    EXPECT_EQ(result.rf_linking_time, 0);
}

TEST(NuphySleepPolicy, WirelessDisconnectTimeoutRequestsSleep) {
    auto policy_state              = default_state();
    policy_state.transport         = NUPHY_SLEEP_TRANSPORT_WIRELESS;
    policy_state.inactivity_time   = 0;
    policy_state.rf_connected      = false;
    policy_state.rf_disconnected   = true;
    policy_state.rf_disconnect_time = NUPHY_SLEEP_POLICY_RF_DISCONNECT_TIMEOUT;

    const auto result = run_policy(policy_state);

    EXPECT_EQ(result.action, NUPHY_SLEEP_ACTION_SLEEP_REQUEST);
    EXPECT_TRUE(result.goto_sleep);
    EXPECT_EQ(result.rf_disconnect_time, NUPHY_SLEEP_POLICY_RF_DISCONNECT_TIMEOUT + 1);
}

TEST(NuphySleepPolicy, WakePreparationBlocksNewSleepRequest) {
    auto policy_state              = default_state();
    policy_state.transport         = NUPHY_SLEEP_TRANSPORT_WIRELESS;
    policy_state.inactivity_time   = policy_state.sleep_timeout;
    policy_state.goto_sleep        = true;
    policy_state.wakeup_prepare    = true;

    const auto result = run_policy(policy_state);

    EXPECT_EQ(result.action, NUPHY_SLEEP_ACTION_WAKE_PREPARATION);
    EXPECT_FALSE(result.goto_sleep);
    EXPECT_TRUE(result.wakeup_prepare);
}

TEST(NuphySleepPolicy, WakePreparationClearsInactivity) {
    auto policy_state              = default_state();
    policy_state.inactivity_time   = 42;
    policy_state.wakeup_prepare    = true;

    const auto result = run_policy(policy_state);

    EXPECT_EQ(result.action, NUPHY_SLEEP_ACTION_WAKE_PREPARATION);
    EXPECT_EQ(result.inactivity_time, 0);
}

TEST(NuphySleepPolicy, DisabledSleepPreservesState) {
    auto policy_state              = default_state();
    policy_state.sleep_toggle      = false;
    policy_state.inactivity_time   = 42;
    policy_state.usb_suspend_debounce = 7;

    const auto result = run_policy(policy_state);

    EXPECT_EQ(result.action, NUPHY_SLEEP_ACTION_NONE);
    EXPECT_EQ(result.inactivity_time, policy_state.inactivity_time);
    EXPECT_EQ(result.usb_suspend_debounce, policy_state.usb_suspend_debounce);
    EXPECT_FALSE(result.goto_sleep);
}
