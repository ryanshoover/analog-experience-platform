# Analog Experience Platform (AXP)
The Great and Mighty Analog Experience Platform

## WIFI
network: NotAPineapple
password: digitalthis

## Pin mapping

### Digital Pins
Success Light = D12
Failure Light = D11
Lasers switch = D10
Sharks switch = D9
Missle Launch switch = D8
Flame sensor = D7
Fire alarm = D6
Toilet flush = D5
Pull chain #1 = D4
Pull chain #2 = D3
Pull chain #3 = D2
Trigger color change = D14

### Analog Pins

Red dial = A0
Green dial = A1
Blue dial = A2
Alcohol sensor = A3

## Actions

Laser switch → Lasers fly across the site from random directions
Sharks switch → Shark fin back and forth across bottom of site
Lasers + Sharks switches → Sharks with frickin lasers on their heads attack the site
“Do not push” Missile Launch → Website cracks like glass. Message pops up saying “You pushed it, didn’t you”
Flame sensor → Hack the site
Fire alarm → Unhack the site
Breathalyzer → Content changes to drunk-speak
Pull chain → Change font size (12px, 18px, 24px, 36px)
Color knobs → Change body (header?) background color
Toilet flush → Visually flush all content on the page

## Needed CLI commands

```bash
wp axp toggle EFFECT
wp axp on EFFECT
wp axp off EFFECT
wp axp set EFFECT VALUE

wp axp toggle sharks
wp axp toggle lasers
wp axp toggle missle-launch
wp axp on alcohol
wp axp off alcohol
wp axp on flame
wp axp off flame
wp axp on toilet
wp axp set color HEX
wp axp set pullchain 1
wp axp set pullchain 2
wp axp set pullchain 3
wp axp set pullchain 0
```
