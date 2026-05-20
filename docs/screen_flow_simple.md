# TimeDisk screen flow (simplified)

Screens only — no actions, events, or decision nodes. See [screen_flow.md](screen_flow.md) for the full diagram (startup WiFi wizards are conditional). Data and modes: [data_model.md](data_model.md) · [mode_flow.md](mode_flow.md).

```mermaid
flowchart TB
    power_up([Power Up])
    splash("Splash Screen")
    startup_wizard_theme("Startup Wizard: Theme")
    startup_wizard_ssid("Startup Wizard: WiFi SSID")
    startup_wizard_password("Startup Wizard: WiFi Password")
    startup_wizard_timezone("Startup Wizard: Timezone")
    loading("Loading / Startup")
    tod_bright("Time of Day")
    menu("Main Menu")
    settings("Settings")
    timer_duration("Timer: Set Duration")
    timer_style("Timer: Set Style")
    timer_bright("Timer")
    sleep_wake("Sleep: Set Wake up Time")
    sleep_rest_end("Sleep: Set Rest End Time")
    sleep_wind_down("Sleep: Set Wind Down Time")
    rest_end("Rest: Set Rest End Time")
    rest_wind_down("Rest: Set Wind Down Time")

    %% Boot (theme/WiFi/timezone wizards skipped when already configured in NVS)
    power_up --> splash --> startup_wizard_theme --> startup_wizard_ssid --> startup_wizard_password --> loading
    loading --> startup_wizard_timezone
    startup_wizard_timezone --> tod_bright
    loading --> tod_bright
    loading --> settings

    %% Time of day
    tod_bright --> menu

    %% Main menu
    menu --> settings
    menu --> tod_bright
    menu --> timer_duration
    menu --> sleep_wake
    menu --> rest_end
    settings --> menu
    settings --> loading

    %% Sleep setup
    sleep_wake --> sleep_rest_end --> sleep_wind_down --> tod_bright
    sleep_wake --> menu
    sleep_rest_end --> sleep_wake
    sleep_wind_down --> sleep_rest_end

    %% Rest setup
    rest_end --> rest_wind_down --> tod_bright
    rest_end --> menu
    rest_wind_down --> rest_end

    %% Timer
    timer_duration --> timer_style --> timer_bright
    timer_duration --> menu
    timer_style --> timer_duration
    timer_bright --> menu

    classDef screen fill:#c8e6c9,stroke:#2e7d32,color:#1b5e20
    classDef start fill:#b2ebf2,stroke:#00838f,color:#006064

    class splash,startup_wizard_theme,startup_wizard_ssid,startup_wizard_password,startup_wizard_timezone,loading,tod_bright,menu,settings,timer_duration,timer_style,timer_bright,sleep_wake,sleep_rest_end,sleep_wind_down,rest_end,rest_wind_down screen
    class power_up start
```
