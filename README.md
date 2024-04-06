# Insomnia Behavior for ZMK

This a module can keep the board awake if BLE is connected. It is used to postpone entering into sleep mode on split peripheral. Majorly, it is designed to avoid continuous BLE Advertistment scanning on split central, if any peripheral go to sleep in a multi-peripheral setup. A key press behavior is also designed to on/off insomnia mode and go into sleep mode immediately.

## What it does

It run a timer in background, which ping `activity_event_listener` after check the shield is connected to a host.
- If enabling on split peripheral, it checks connection state via `bool zmk_split_bt_peripheral_is_connected(void)`.
- If enabling on central, it checks connection state via `bool zmk_ble_active_profile_is_connected(void)`.

## Installation

Include this modulr on your ZMK's west manifest in `config/west.yml`:

```yaml
manifest:
  remotes:
    #...
    # START #####
    - name: badjeff
      url-base: https://github.com/badjeff
    # END #######
    #...
  projects:
    #...
    # START #####
    - name: zmk-behavior-insomnia
      remote: badjeff
      revision: main
    # END #######
    #...
```

And update `shield.conf` on *peripheral shield*.
```
CONFIG_ZMK_SLEEP=y

# enable insomnia on peripheral shield
CONFIG_ZMK_INSOMNIA=y

# go to sleep ONLY AFTER 16 seconds on disconnected to central
# central disconnection imply that it just fall asleep.
CONFIG_ZMK_IDLE_SLEEP_TIMEOUT=16000

# set a ping before attempting to sleep (minus 1 second, 1000ms)
CONFIG_ZMK_INSOMNIA_PING_INTERVAL=15000

# set a ping before attempting to sleep AFTER 15 (minus 1 second, 1000ms)
CONFIG_ZMK_INSOMNIA_PING_INTERVAL=15000

# enable insomnia on boot, or after wakeup
CONFIG_ZMK_INSOMNIA_PING_ON_START=y
```

Also, update 'shield.keymap' if want manual control
```
#include <behaviors/insomnia.dtsi>
#include <dt-bindings/zmk/insomnia.h>

/{
    behaviors {
        &isa {
            /*
              Set delay before heading to sleep.
              e.g. To allow LED / e-ink display to clear the screen before power down
            */
            sleep-delay-ms = <200>;
        };
    };

    keymap {
        compatible = "zmk,keymap";
        DEF_layer {
            bindings = <
              &isa ISA_ON   /* enable stay awake */
              &isa ISA_OFF  /* disable stay awake, normal sleep mode */
              &isa ISA_SLP  /* go into sleep mode immediately */
            >;
        };
    };

};
```
