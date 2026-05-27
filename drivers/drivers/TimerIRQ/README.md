
# TimerIRQ
Timer IRQ dispatch infrastructure.

## Description
The STM32F407 timer interrupt lines are shared: TIM1's BRK/UP/TRG_COM vectors are
multiplexed with TIM9/10/11, TIM8's with TIM12/13/14, and TIM6's with the DAC
underrun interrupt. If each timer driver defined its own `extern "C"` ISR symbol,
two drivers on the same shared line would produce a duplicate-symbol link error.

`TimerIRQ` owns the **only** copy of every timer ISR symbol and fans each vector
out to a per-timer dispatch slot. Timer drivers (`GenericTimer`, `BasicTimer`, `PWM`)
register their HAL dispatch slot at `Init()` and unregister it at `Sleep()`. The
user-facing driver API is unchanged.

## Requirements
- ST Microelectronics STM32F407G-DISC1 (can be ported easily to other ST microcontrollers)
- C++14

## Notes
This is internal infrastructure consumed by `GenericTimer`, `BasicTimer`, and `PWM`.
Application code does not interact with `TimerIRQ` directly.
Each shared vector calls every timer slot mapped to it; the registered handler
(`HAL_TIM_IRQHandler(&handle)`) demuxes by enabled+pending flag, so a slot firing on
a timer that did not raise an interrupt is a silent no-op — the same behaviour as
before this infrastructure was introduced.
