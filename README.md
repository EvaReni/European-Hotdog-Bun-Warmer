# European Hotdog Bun Warmer

## The Idea

The European Hotdog Bun Warmer is a custom-built, dual-zone heating system designed to heat and maintain two aluminum heating sections at a controlled temperature.

The system uses two cartridge heaters and two NTC thermistors to independently monitor and control each heating zone. An LGT8F328 microcontroller runs the temperature-control system using PID control to regulate the heater outputs. A 16 × 2 I2C LCD displays the temperature setpoint and heater output, while two buttons allow the target temperature to be adjusted.

The warmer combines a custom-fabricated aluminum exterior, a wooden enclosure, aluminum heating components, and 3D-printed mounting parts. The electronics, thermal design, and mechanical construction work together to create a complete heating system.

The default temperature setpoint is **80°C**, adjustable from **30°C to 120°C**. Additional features include temperature filtering, optional bang-bang control, audible feedback, and a heater safety-monitoring system.

**Author:** Eva Mateev  
**License:** CC BY 4.0

## What We Are Using (Parts List)

### Heating System

- **12V 40W Stainless Steel Cartridge Heaters (2):** Heating elements used to heat the two aluminum heating sections.
- **100kΩ NTC Thermistors (2):** Measure the temperature of each heating zone. Both thermistors use a Beta value of 3950.
- **6061-T6511 Aluminum Round Bar (2):** Each bar measures 1 inch in diameter and 13 inches long and provides a thermal path from the cartridge heaters.
- **6061-T651 Aluminum Sheet (1):** A 12 × 16 inch sheet, 1/16 inch thick, bent to form the outer metal layer of the warmer.

### Enclosure & Mechanical Components

- **Wooden Box:** Forms the main enclosure beneath the folded aluminum exterior and supports the internal components.
- **3D-Printed Driver Holders:** Custom-designed holders used to secure the heater driver modules to the wooden enclosure.
- **3D-Printed ViewSonic Socket:** Custom-designed ViewSonic socket to power the whole unit with the ViewSonic power adapter.
- **12V DC Power Supply (4.16A):** Repurposed ViewSonic power adapter used to supply power to the warmer.

### Electronics & Controls

- **LGT8F328 Microcontroller (1):** Reads the thermistors, runs the temperature-control algorithms, and manages the heater outputs, LCD, buttons, and buzzer.
- **XY-MOS driver (2):** Control the power delivered to the cartridge heaters using the microcontroller's PWM outputs.
- **16 × 2 I2C LCD (1):** Displays the temperature setpoint and heater output.
- **Push Buttons (2):** Increase or decrease the shared temperature setpoint.
- **3 Legged Toggle Switch:** Powers whole unit on and off
- **Buzzer (1):** Provides audible feedback for important system events.
- **LED Voltage & Current Meter (1):** Displays the power supply voltage and current draw.

## Important Development Notes & Libraries

- **Development Environment:** The project is programmed for the LGT8F328 using the Arduino development environment.
- **Thermistor Configuration:** Both thermistors are 100kΩ NTC sensors with a Beta value of 3950.
- **Temperature Filtering:** The software averages 20 ADC readings before applying an exponential moving average (EMA) with α = 0.2.
- **LCD Configuration:** The 16 × 2 I2C LCD uses address `0x27`.
- **PID Configuration:** Each heating zone has an independent PID controller, but both controllers share the same temperature setpoint.
- **PWM Configuration:** Timer 1 is configured for phase-correct 8-bit PWM with a prescaler of 1024, producing an approximately 30.5 Hz PWM frequency.
- **Buzzer Timing:** Buzzer sounds use `millis()`-based timing to avoid blocking the main temperature-control loop.
- **Safety Monitoring:** The `performSafetyCheck()` function is included, its called inside `loop()`.
- **Libraries:** The main sketch uses `Wire.h`, `LiquidCrystal_I2C.h`, and `math.h`, along with the project-local headers `pitches.h` and `songDefs.h`. The LCD library version depends on the one installed in the development environment.

## Wiring & Hardware Pin Mapping

The electronics are documented using a KiCad schematic, which can be included with the project files.

### Microcontroller Pin Mapping

| **Component** | **LGT8F328 Pin** | **Notes** |
|---|---|---|
| Thermistor 1 | A0 | Temperature sensor for heating zone 1 |
| Thermistor 2 | A1 | Temperature sensor for heating zone 2 |
| Heater 1 | D9 | Heating zone 1 output |
| Heater 2 | D10 | Heating zone 2 output |
| Plus Button | D3 | Increases temperature setpoint |
| Minus Button | D4 | Decreases temperature setpoint |
| Buzzer | D8 | Audible system feedback |
| LCD Display | I2C | I2C communication |
| LCD Backlight | D6 | Controls LCD backlight |

### Thermistor Connections

Each NTC thermistor connects to an analog input so the microcontroller can measure the corresponding heating zone's temperature.

- **Thermistor 1:** A0
- **Thermistor 2:** A1

The measured values are converted into temperatures and filtered before being passed to the PID controllers.

### Heater Connections

The two heater driver modules receive their control signals from the microcontroller's PWM outputs.

- **Heater 1:** D9
- **Heater 2:** D10

Each output controls its own heating zone.

### LCD, Buttons & Buzzer

- **LCD:** Connected through the I2C interface.
- **Plus Button:** D3.
- **Minus Button:** D4.
- **Buzzer:** D8.
- **LCD Backlight:** D6.

The buttons use the microcontroller's internal pull-up resistors.

### KiCad Schematic

The KiCad project files provide the electrical schematic and component connections. Refer to the schematic when assembling or modifying the electronics.

> **Power supply note:** The cartridge heaters are rated for 12V. Verify the actual power supply voltage and the wiring of the heater drivers before connecting power. A 24V supply must not be connected directly to 12V-rated heaters.

## Temperature Measurement

The two 100kΩ NTC thermistors use a Beta value of **3950**.

The microcontroller measures the thermistor voltage through its analog inputs and calculates the temperature from the corresponding resistance using the Beta temperature equation.

To reduce fluctuations in the readings, the software uses two stages of filtering:

1. **ADC averaging:** 20 analog readings are averaged to reduce measurement noise.
2. **Exponential moving average (EMA):** The averaged temperature is smoothed using α = 0.2.

The filtered temperatures are then passed to the PID controllers.

## PID Control

Each heating zone has its own independent PID controller. This allows the system to adjust each heater's output according to its own measured temperature.

Both zones use the same target temperature, which can be adjusted using the two push buttons.

### PID Parameters

| **Parameter** | **Heater 1** | **Heater 2** |
|---|---:|---:|
| Kp | 50 | 50 |
| Ki | 1.5 | 1.5 |
| Kd | 2 | 2 |

The PID controllers update once every **1 second**.

The calculated heater output is limited to a range of **0–255** and used as the PWM value for the corresponding heater driver.

### Integral Windup Protection

The software limits integral windup to prevent the integral term from growing excessively while the system is heating.

During startup, the integral term is also reset while the measured temperature is below **40°C**. These measures help control the integral contribution during the initial heating period.

## Bang-Bang Control

The software also includes an optional bang-bang temperature-control mode.

Instead of continuously adjusting heater output using PID, bang-bang control switches the heater on and off using a **2°C hysteresis** around the target temperature.

The control mode is selected through the `useBangBang` variable:

```cpp
useBangBang = false;
```

PID control is enabled by default.

## LCD Display

The 16 × 2 I2C LCD displays the target temperature and heater output.

- **I2C address:** `0x27`

Example display:

```text
80C/80C    80C/80C
H1:150     CH2:175
```

The first line displays the setpoint for both heating zones. The second line displays the current output of each heater.

The output values represent the controller's PWM command, ranging from 0 to 255.

## Temperature Adjustment

Two push buttons control the shared temperature setpoint.

| **Button** | **Pin** |
|---|---|
| Plus | D3 |
| Minus | D4 |

The buttons use the microcontroller's internal pull-up resistors.

- **Short press:** Changes the setpoint by one step.
- **Long press:** Holding a button for approximately 700 ms activates repeated adjustments at approximately 100 ms intervals.

Both heating zones use the same setpoint and are adjusted together.

### Setpoint Range

- **Minimum:** 30°C
- **Maximum:** 120°C
- **Default:** 80°C

The software keeps the setpoint within the defined minimum and maximum temperatures.

## Buzzer

The buzzer is connected to **D8** and provides audible feedback for important system events.

| **Event** | **Sound** |
|---|---|
| Target temperature reached | `DONE` |
| Safety failure | `FAIL` |
| Safety reset | `RESET` |

The sounds are played using a non-blocking `millis()` timing system. This allows the temperature-control system to continue operating while the buzzer plays.

### Power Monitoring Display

An LED voltage and current meter is connected to the power supply to display its voltage and current draw during operation. This provides a convenient way to observe the electrical power being used while the warmer is running.

The meter is an additional monitoring feature rather than part of the temperature-control system.

## Safety System

The program includes a safety-monitoring function designed to detect a heater that receives a high PWM output but fails to produce a sufficient increase in temperature over a defined period.

Each heating zone has its own independent halt state. If a fault is detected, the affected heater can be stopped while the other zone continues operating.

If both heaters are halted, the LCD displays:

```text
-System halted!-
```

Both temperature buttons can be pressed simultaneously to reset the safety state.

### Safety Check Status

The `performSafetyCheck()` function is included in the program, but its call inside `loop()` is currently commented out.

Therefore, the temperature-rise safety check is **not currently active** unless the function call is enabled.

> **Safety warning:** The software monitoring feature should not be treated as a substitute for independent hardware protection against overheating or electrical faults.

## Thermal Design & Model

The heating system uses aluminum components to transfer and distribute heat from the cartridge heaters.

The two 6061-T6511 aluminum round bars provide a thermal path from the heaters, while the 6061-T651 aluminum sheet forms the outer metal structure.

A thermal model was created in FreeCAD to examine the expected temperature distribution through the aluminum components.

### FreeCAD Thermal Model

The model illustrates the expected heat distribution through the aluminum heating structure and supports the thermal design of the warmer.

*The FreeCAD thermal model was created by my father as part of the thermal design of the project.*

## Physical Design & Construction

The warmer is built around a wooden box enclosure covered by a folded aluminum exterior. The mechanical design brings together the heating components, electrical hardware, and custom mounting parts into a single assembly.

### Wooden Enclosure

The wooden box forms the main supporting structure of the warmer. The folded aluminum sheet covers the exterior, while the enclosure provides space to mount and position the internal electronics.

The wooden structure also provides attachment points for the custom 3D-printed driver holders.

### Aluminum Exterior & Metal Fabrication

The outer layer is made from a 6061-T651 aluminum sheet measuring 12 × 16 inches and 1/16 inch thick.

The sheet was bent to fit around the wooden box. To achieve the required shape, we used a combination of wood, a manual hydraulic press, and clamps.

After the sheet was folded into shape, we used brazing to join the relevant sections of the metal exterior.

*My father and I worked together on the metal fabrication, including bending and brazing. We shared the work, taking turns operating the equipment and performing the brazing.*

### Heater Driver Mounting

Custom 3D-printed holders were designed to attach the heater driver modules to the wooden enclosure.

These holders keep the drivers securely positioned within the box rather than leaving them unsupported. The corresponding FreeCAD models can be included in the repository if available.

### Power Supply Opening

An opening was incorporated into the enclosure to accommodate the power supply connection. The opening allows the supply cable or connector to pass through the wooden structure.

## CAD Files & Project Documentation

The repository includes design files and documentation for the electronic and mechanical components of the European Hotdog Bun Warmer.

- **KiCad:** Electrical schematic and related design files.
- **FreeCAD Thermal Model:** Model of the expected temperature distribution through the aluminum heating structure.
- **FreeCAD Mechanical Models:** Available enclosure, mounting-holder, and other mechanical design files.
- **3D-Printed Parts:** CAD files for the custom driver holders and other printed components, where available.
- **Finished Assembly Photo:** Photograph of the completed European Hotdog Bun Warmer.

These files document the design and construction of the warmer, from its electronic control system to its physical heating structure.

## License

This project is licensed under the **Creative Commons Attribution 4.0 International (CC BY 4.0)** license.

You are free to share and adapt this work, provided that appropriate credit is given to the original author.

**Author:** Eva Mateev  
**License:** CC BY 4.0
