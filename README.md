# Tester PCB Business Card

A PCB Business Card designed by a tester, for tester (QA Engineers)

I've always liked the PCB business cards I've seen people design. They often have an interesting story about the why and the how, and sometimes even lead to a job offer as a result!

> Note: For any recruiters or prospective employers who end up here, I'm a software tester specializing in WebUI test automation. Electronics and hardware is a hobby (and occasionally a freelance gig in the Creative Technologist/Arts and Technology space)

I wanted to give it a go but couldn't think of anything compelling that was related to my primary industry (QA, Software Testing & Test Automation). Then I saw a recent [pong example on Hackaday](https://hackaday.com/2025/08/13/pcb-business-card-plays-pong-attracts-employer/) which made me wonder whether the author had done much testing, and that sparked the idea of creating a PCB business card all about testing.

There are 5 test cases, each one designed to be a little bit harder than the previous one and to focus on a different class of test. The hardware is somewhat limited, so how well each of the tests achieves its intended goal is open for debate and I welcome feedback from actual testers!

> Note: There may be minor differences between the simulator and the actual hardware due to implementation details or I changed direction while working on the firmware and haven't gotten round to updating the simulator.

So here is Rev A:

![Tester PCB Preview](./resources/tester_pcb_business_card.png)

This is based on the CH32V003F4U6. A 27¢ micro controller with a one-wire programmer that costs less than $8. I had no experience with this when I started this project, but cost was a big factor with the card. I'm aiming for sub $2 per unit so I can a) afford to get a handful made and b) give them away at conferences and networking events.

This repo includes:

# Hardware

The [schematic](./resources/schematic.pdf), PCB design, BoM and JLCPCB ready production files [Kicad 9 files here](./hardware/)

# Software

## Simulator

Simulator [README](./software/simulator)

You can play with the simulator here: [Simulator Demo](./assets/tester_pcb_business_card_simulator.gif)

## Firmware

Uses platformio and the ch32fun framework.

[Main firmware](./software/firmware/tester_runtime/): The main firmware containing tests, test case runner, LED control, button functions etc.

[Board check](./software/firmware/board_check/): Simple test firmware to confirm LEDs and buttons are connected and working. Mainly used for testing hand assembled prototype boards.

[LED Tune](./software/firmware/led_tune/): Simple firmware to allow manual tuning of resistor values. shouldn't need this again unless the LEDs are changed for ones that are significantly different.

# Licenses

Copyright Mark Benson 2025

The hardware is licensed under the CERN-OHL-S v2 license.
The software is licensed under the MIT license.

Usual disclaimer - No warranty implied or given for any use or outcome of any of the available materials, etc.
