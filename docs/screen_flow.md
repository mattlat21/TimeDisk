# TimeDisk Screen Flow

**Related docs:** [data_model.md](data_model.md) · [mode_flow.md](mode_flow.md) · [adult_authentication.md](adult_authentication.md) · [data_flow.md](data_flow.md) · [wireframes](wireframes/README.md)

Simplified screens-only diagram: [screen_flow_simple.md](screen_flow_simple.md).

```mermaid
flowchart TB
    %% Boot
    power_up([Power Up])
    splash("Splash Screen")
    startup_wizard_theme("Startup Wizard: Theme")
    startup_wizard_ssid("Startup Wizard: WiFi SSID")
    startup_wizard_password("Startup Wizard: WiFi Password")
    startup_wizard_timezone("Startup Wizard: Timezone")
    loading("Loading / Startup")

    %% Screens (green)
    menu("Main Menu")
    tod_dim("Time of Day (Dim)")
    tod_bright("Time of Day with Menu Button (Bright)")
    aa_start("Adult Authentication")
    settings("Settings (Main)")
    timer_duration("Timer: Set Duration")
    timer_style("Timer: Set Style")
    timer_bright("Timer with End Button (Bright)")
    timer_dim("Timer (Dim)")
    aa_end("Adult Authentication")
    confirm("Are you sure?")
    sleep_set_wake_up_time("Sleep: Set Wake up Time")
    sleep_rest_rest_end_time("Sleep: Set Rest End Time")
    sleep_set_wind_down_time("Sleep: Set Wind Down Time")
    rest_set_rest_end_time("Rest: Set Rest End Time")
    rest_set_wind_down_time("Rest: Set Wind Down Time")
    timer_triggered("Timer Triggered")

    %% User actions (blue)
    tap1("Tap Screen")
    btn_press("Menu Button Press")
    cancel_start("Cancel Button")
    tap2("Tap Screen")
    end_btn("End Button")
    cancel_end("Cancel Button")
    confirm_no("No")
    confirm_yes("Yes")
    selected_main_menu_item("Selected Main Menu Item")
    loading_settings("Settings")
    timer_done_ok("OK")
    startup_wizard_theme_ok("Next Button")
    startup_wizard_ssid_ok("Next Button")
    startup_wizard_password_ok("Next Button")
    startup_wizard_timezone_ok("Next Button")

    %% User Back Buttons
    back_btn_settings("Back Button")
    back_style("Back Button")
    back_duration("Back Button")
    sleep_set_wake_up_time_back_button("Back Button")
    sleep_rest_rest_end_time_back_button("Back Button")
    sleep_set_wind_down_time_back_button("Back Button")
    rest_set_rest_end_time_back_button("Back Button")
    rest_set_wind_down_time_back_button("Back Button")

    %% User Next Buttons
    ok_duration("Next Button")
    ok_style("Next Button")
    sleep_set_wake_up_time_next_button("Next Button")
    sleep_rest_rest_end_time_next_button("Next Button")
    sleep_set_wind_down_time_next_button("Next Button")
    rest_set_rest_end_time_next_button("Next Button")
    rest_set_wind_down_time_next_button("Next Button")

    %% System events (purple)
    splash_timeout("Splash Timeout")
    timeout_tod("T.O.D. Dim Timeout")
    fail_start("Fail")
    pass_start("Pass")
    timeout_aa_start("A.A. Timeout")
    dim_timeout("Timer Dim Timeout")
    fail_end("Fail")
    timeout_aa_end("A.A. Timeout")
    pass_end("Pass")
    main_menu_timeout("Main Menu Timeout")
    timer_done_timeout("Timer Done Timeout")

    %% Decision (orange)
    which_main_menu_item_selected{{"Menu Item?"}}
    time_obtained{{"Current Time Known?"}}
    settings_is_time_loaded{{"Current Time Known?"}}
    is_timer_triggered_bright{{"Timer Triggered"}}
    is_timer_triggered_dim{{"Timer Triggered"}}
    theme_unset{{"theme_set false?"}}
    wifi_ssid_missing{{"wifi_ssid blank or null?"}}
    wifi_password_null{{"wifi_password null?"}}
    timezone_unset{{"timezone_set false?"}}

    %% Helpful Labels
    timer_done([Back to T.o.D.])
    timer_back_to_menu([Back to Menu])
    startToD(["Start Time of Day"])

    %% Boot sequence
    subgraph Boot
        power_up --> splash
        splash --> splash_timeout --> theme_unset
        theme_unset --> |Yes| startup_wizard_theme
        theme_unset --> |No| wifi_ssid_missing
        startup_wizard_theme --> startup_wizard_theme_ok --> wifi_ssid_missing
        wifi_ssid_missing --> |Yes| startup_wizard_ssid
        wifi_ssid_missing --> |No| wifi_password_null
        startup_wizard_ssid --> startup_wizard_ssid_ok --> wifi_password_null
        wifi_password_null --> |Yes| startup_wizard_password
        wifi_password_null --> |No| loading
        startup_wizard_password --> startup_wizard_password_ok --> loading
        loading --> time_obtained --> |No| loading
        loading --> loading_settings
    end
    time_obtained --> |Yes| timezone_unset
    timezone_unset --> |Yes| startup_wizard_timezone
    timezone_unset --> |No| tod_bright
    startup_wizard_timezone --> startup_wizard_timezone_ok --> tod_bright
    loading_settings --> settings

    %% Idle and authentication
    subgraph Time Of Day
        tod_dim --> tap1 --> tod_bright
        tod_bright --> timeout_tod --> tod_dim
        tod_bright --> btn_press --> aa_start
        subgraph Adult Authentication
            aa_start --> fail_start
            aa_start --> timeout_aa_start
            aa_start --> cancel_start
            aa_start --> pass_start
        end
        fail_start --> aa_start
        timeout_aa_start --> tod_bright
        cancel_start --> tod_bright
    end
    startToD --> tod_bright

    %% Menu
    subgraph Main Menu
        menu --> selected_main_menu_item --> which_main_menu_item_selected
        menu --> main_menu_timeout
    end
    which_main_menu_item_selected --> |Settings| settings
    which_main_menu_item_selected --> |Back| tod_bright
    which_main_menu_item_selected --> |Start Timer| timer_duration
    which_main_menu_item_selected --> |Sleep| sleep_set_wake_up_time
    which_main_menu_item_selected --> |Rest| rest_set_rest_end_time
    main_menu_timeout --> tod_bright


    %% Back to Menu
    settings_is_time_loaded --> |Yes| menu
    sleep_set_wake_up_time_back_button --> menu
    rest_set_rest_end_time_back_button --> menu
    pass_start --> menu
    timer_back_to_menu --> menu

    %% Settings (hub + six sub-panels; Cancel wedge on hub, Save/Cancel wedges on each panel)
    subgraph Settings
        settings --> back_btn_settings --> settings_is_time_loaded
    end
    settings_is_time_loaded --> |No| loading

    Firmware (`ui_screen_settings.c`): single `UI_SCREEN_SETTINGS` with hidden sub-panels (Colours, **Networking**, Timezone, Schedule, Timeouts, Adult Authentication). Hub **Cancel wedge** returns to menu when time is known, else loading. Sub-panels **Save** persist via `app_nvs_save_*` and return to hub; **Cancel** restores that section from the snapshot taken on `ui_screen_settings_on_show()`.

    **Networking** sub-panel: list of **Wi‑Fi Name**, **Wi‑Fi Password**, and **NTP** → per-field edit (wizard-style keyboard, purple field bar, **Cancel** / **Save** wedges). Field **Save** writes that value to NVS via `app_config_save_network()` and returns to the list; list **Save** returns to the settings hub (draft already persisted per field).

### Settings: Networking (sub-panel pages)

All states live on `UI_SCREEN_SETTINGS` (hidden LVGL sub-panels in `ui_screen_settings.c`).

```mermaid
flowchart TB
    settings_hub("Settings hub")
    settings_networking("Networking — submenu")
    settings_net_edit("Network field edit")
    settings_hub -->|"Networking button"| settings_networking
    settings_networking -->|"Wi‑Fi Name / Wi‑Fi Password / NTP"| settings_net_edit
    settings_net_edit -->|"Cancel or Save wedge"| settings_networking
    settings_networking -->|"Cancel or Save wedge"| settings_hub
```

| Page | User actions | Persistence |
| ---- | ------------ | ------------- |
| Networking list | Pick a field; **Cancel** / **Save** wedges → settings hub | **Save** on list is optional (draft already saved per field); **Cancel** reverts all network fields from snapshot |
| Field edit | On-screen keyboard; **Cancel** discards edits for that field; **Save** writes field → NVS (`app_config_save_network`) → list | Same fields as [startup Wi‑Fi wizard](data_model.md#startup-wizard-boot-only) plus `ntp_server` |

    %% Time of Day Setup
    subgraph Set Sleep Time
        sleep_set_wake_up_time --> sleep_set_wake_up_time_next_button --> sleep_rest_rest_end_time
        sleep_set_wake_up_time --> sleep_set_wake_up_time_back_button
        sleep_rest_rest_end_time --> sleep_rest_rest_end_time_next_button --> sleep_set_wind_down_time
        sleep_rest_rest_end_time --> sleep_rest_rest_end_time_back_button --> sleep_set_wake_up_time
        sleep_set_wind_down_time --> sleep_set_wind_down_time_next_button
        sleep_set_wind_down_time --> sleep_set_wind_down_time_back_button --> sleep_rest_rest_end_time
    end
    sleep_set_wind_down_time_next_button --> startToD

    subgraph Set Rest Time
        rest_set_rest_end_time --> rest_set_rest_end_time_next_button --> rest_set_wind_down_time
        rest_set_rest_end_time --> rest_set_rest_end_time_back_button
        rest_set_wind_down_time --> rest_set_wind_down_time_next_button
        rest_set_wind_down_time --> rest_set_wind_down_time_back_button --> rest_set_rest_end_time
    end
    rest_set_wind_down_time_next_button --> startToD

    %% Timer setup
    subgraph Timer
        timer_duration --> ok_duration --> timer_style
        timer_duration --> back_duration
        timer_style --> ok_style --> timer_bright
        timer_style --> back_style --> timer_duration

        %% Active timer
        timer_bright --> dim_timeout --> timer_dim
        timer_dim --> tap2 --> timer_bright
        timer_bright --> end_btn --> aa_end

        subgraph Adult Authentication
            aa_end --> fail_end --> aa_end
            aa_end --> timeout_aa_end
            aa_end --> cancel_end
            aa_end --> pass_end
        end
        timeout_aa_end --> timer_bright
        cancel_end --> timer_bright
        pass_end --> confirm
        confirm --> confirm_no --> timer_bright
        confirm --> confirm_yes

        timer_bright --> is_timer_triggered_bright --> |No| timer_bright
        timer_dim --> is_timer_triggered_dim --> |No| timer_dim

        is_timer_triggered_bright --> |Yes| timer_triggered
        is_timer_triggered_dim --> |Yes| timer_triggered

        timer_triggered --> timer_done_ok --> timer_done
        timer_triggered --> timer_done_timeout --> timer_done

        back_duration --> timer_back_to_menu
        confirm_yes --> timer_back_to_menu

    end
    timer_done --> tod_bright

    classDef screen fill:#c8e6c9,stroke:#2e7d32,color:#1b5e20
    classDef action fill:#bbdefb,stroke:#1565c0,color:#0d47a1
    classDef event fill:#e1bee7,stroke:#6a1b9a,color:#4a148c
    classDef decision fill:#ffe0b2,stroke:#e65100,color:#bf360c
    classDef helpful_label fill:#b2ebf2,stroke:#00838f,color:#006064

    class splash,startup_wizard_theme,startup_wizard_ssid,startup_wizard_password,startup_wizard_timezone,loading,tod_dim,tod_bright,aa_start,menu,settings,timer_duration,timer_style,timer_bright,timer_dim,aa_end,confirm,sleep_set_wake_up_time,sleep_rest_rest_end_time,sleep_set_wind_down_time,rest_set_rest_end_time,rest_set_wind_down_time,timer_triggered screen

    class tap1,btn_press,cancel_start,ok_duration,back_btn_settings,back_duration,ok_style,back_style,tap2,end_btn,cancel_end,confirm_no,selected_main_menu_item,confirm_yes,sleep_set_wake_up_time_back_button,sleep_rest_rest_end_time_back_button,sleep_set_wind_down_time_back_button,sleep_set_wake_up_time_next_button,sleep_rest_rest_end_time_next_button,sleep_set_wind_down_time_next_button,rest_set_rest_end_time_back_button,rest_set_wind_down_time_back_button,rest_set_rest_end_time_next_button,rest_set_wind_down_time_next_button,loading_settings,timer_done_ok,startup_wizard_theme_ok,startup_wizard_ssid_ok,startup_wizard_password_ok,startup_wizard_timezone_ok action

    class splash_timeout,timeout_tod,fail_start,pass_start,timeout_aa_start,dim_timeout,fail_end,timeout_aa_end,pass_end,main_menu_timeout,timer_done_timeout event

    class power_up,timer_done,timer_back_to_menu,startToD helpful_label

    class which_main_menu_item_selected,time_obtained,settings_is_time_loaded,is_timer_triggered_dim,is_timer_triggered_bright,theme_unset,wifi_ssid_missing,wifi_password_null,timezone_unset decision
```

## Legend

| Color  | Meaning                           |
| ------ | --------------------------------- |
| Cyan   | Helpful Label                     |
| Green  | Screen                            |
| Blue   | User action or button             |
| Purple | System event or validation result |
| Orange | Decision                          |

## Notes

- **Timeout config fields** (see [data_model.md](data_model.md)): `timeout_splash_sec`, `timeout_tod_dim_sec`, `timeout_aa_sec`, `timeout_main_menu_sec`, `timeout_timer_dim_sec`.
- **Boot** — Power Up → Splash (`timeout_splash_sec`) → optional theme wizard → optional WiFi **Startup Wizard** screens → **Loading / Startup** (WiFi + SNTP) → optional timezone wizard → Time of Day (Bright).
- **Startup Wizard (theme)** — after splash if `theme_set` is false (see [data_model.md](data_model.md#theme)):
  - `startup_wizard_theme` — preset swatches for primary and secondary; live preview on screen ring and sample labels; **Next** writes colours, sets `theme_set`, then continues to WiFi wizards or Loading.
- **Startup Wizard (WiFi)** — shown only when credentials are missing in NVS (see [data_model.md](data_model.md#network)):
  - `startup_wizard_ssid` — if `wifi_ssid` is blank or null; user enters SSID, **Next** writes `wifi_ssid`.
  - `startup_wizard_password` — if `wifi_password` is **null** (never configured); user may enter a passphrase or leave it blank (open network); **Next** writes `wifi_password` (may be empty string). Skipped when password was previously set.
- WiFi wizard order: SSID check first, then password check, then Loading.
- **Startup Wizard (timezone)** — after Loading when `time_valid` (see [data_model.md](data_model.md#timezone)):
  - `startup_wizard_timezone` — if `timezone_set` is false; **Country** and **Location** dropdowns; preview clock (HH:MM) updates in real time as selections change; **Next** writes `timezone_id`, sets `timezone_set`, applies TZ, then navigates to Time of Day (Bright).
  - Not shown until UTC is known (stay on Loading if SNTP fails).
- **T.o.D.** — time-of-day display (idle).
- **A.A.** — adult authentication (PIN / challenge); used when entering the menu from idle and when ending an active timer.
- **menu** ↔ **settings** — settings is reachable from the menu and returns to it.
