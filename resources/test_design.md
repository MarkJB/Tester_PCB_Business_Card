# 🧪 Test Suite Overview

| Test # | Name                        | Type                                      |
|--------|-----------------------------|-------------------------------------------|
| 1      | Button Functionality Check  | Smoke / Sanity Test                       |
| 2      | Input Debounce Validation   | Boundary Value / Timing Test              |
| 3      | Conditional Logic           | Decision Table / Branch Test              |
| 4      | Input Range Check           | Equivalence Partitioning / Threshold Test |
| 5      | Unlock Pattern              | State Transition / Timeout Test           |

---

## 🧪 Test 1: Button Functionality Check

> **Instruction**: Press each button in turn.  
> **Type**: Smoke / Sanity Test

#### Scenario: Validate input button functionality

```gherkin
Given the system state is powered on and in an idle state
When I press each input button in sequence
Then each button press is registed by the system
And the system is ready for further testing
```
---

## 🧪 Test 2: Input Debounce Validation – Boundary Value / Timing Test

> **Type**: Boundary Value / Timing Test  
> **Instruction**: Press button B twice with a specific delay between presses.

### Scenario: Validate system debounce threshold using timed input

```gherkin
Given the system is idle and ready to receive input
When I press button B twice with a delay greater than 100ms
Then the system registers both presses as valid input

Given the system is idle and ready to receive input
When I press button B twice with a delay less than 100ms
Then the system ignores the second press as invalid input
```

### 🧠 What’s Being Tested

> “This test models a system that filters out rapid, repeated signals. The tester must press B twice with a precise delay to determine the system’s debounce threshold.”

- **Boundary**: 100ms (configurable)
- **Pass**: Two presses >100ms apart → both counted
- **Fail**: Two presses <100ms apart → second ignored

### 🔧 Test Logic

| Timing Between Presses | Interpretation             | Test Outcome |
|------------------------|----------------------------|--------------|
| >100ms                 | Treated as valid input     | ✅ Pass      |
| <100ms                 | Treated as bounce → ignored| ❌ Fail      |

### 💡 LED Feedback (Optional)

| Outcome | LED Behavior   |
|---------|----------------|
| Pass    | Solid Green    |
| Fail    | Solid Red      |


---

## 🧪 Test 3: Conditional Logic – Decision Table Test

> **Type**: Decision Table / Branch Test  
> **Instruction**: Press any combination of A, B, and C within 5 seconds.

### Scenario: Evaluate system logic based on input combinations

```gherkin
Given the system is idle and ready to receive input
When the user presses any combination of buttons A, B, and C within a 5-second window
Then the system stores the input state until the timeout expires
And the system evaluates the final input state against its decision table
```

### 🧠 What’s Being Tested

> “This test models decision table logic. The tester must deduce the correct input combination by observing system outcomes and reasoning through partial correctness.”

- **Inputs**: A, B, C
- **Time Limit**: 5 seconds
- **Outcome Types**:
  - ✅ **Pass** → Correct combination
  - ⚠️ **Warning** → Partial match
  - ❌ **Fail** → Incorrect or no input

### 🔧 Test Logic

| A | B | C | Outcome | Reasoning                          |
|---|---|---|---------|------------------------------------|
| 0 | 0 | 0 | Fail    | No input → no correct signal       |
| 0 | 0 | 1 | Warning | C alone → partial match            |
| 0 | 1 | 0 | Warning | B alone → partial match            |
| 0 | 1 | 1 | Pass    | ✅ B + C → correct combo            |
| 1 | 0 | 0 | Fail    | A alone → incorrect input          |
| 1 | 0 | 1 | Warning | A + C → C is good, A is not        |
| 1 | 1 | 0 | Warning | A + B → B is good, A is not        |
| 1 | 1 | 1 | Warning | B + C is good, but A degrades combo|

### 💡 Test Result Feedback

| Outcome | LED Behavior     |
|---------|------------------|
| Pass    | Solid Green      |
| Warning | Blinking Green   |
| Fail    | Solid Red        |


---

## 🧪 Test 4: Input Range Check – Equivalence Partitioning / Threshold Test

###  Scenario: System validates input ranges after timeout

```gherkin
Given the system is idle and ready to receive input
When the user provides values for inputs A, B, and C within a 5-second window
Then the system stores the final input state at the end of the window
And the system evaluates each input independently against its valid range
And the system displays per-input feedback via LEDs based on validity
```

> **Type**: Equivalence Partitioning / Threshold Test  
> **Instruction**: Provide values for A, B, and C within a 5-second window.

### 🧠 What’s Being Tested

> “This test models per-input validation using equivalence partitioning. Each input is evaluated independently against its valid range, reinforcing modular reasoning and threshold awareness. It teaches testers to think in terms of input classes and boundary sensitivity.”

### 📊 Example Outcomes

| A | B | C | Outcome Description                     |
|---|---|---|------------------------------------------|
| 2 | 6 | 3 | ✅ All valid → Green LEDs for A/B/C       |
| 3 | 6 | 3 | ⚠️ A borderline → A blinks green          |
| 0 | 2 | 0 | ❌ B invalid → B red                      |
| 0 | 0 | 0 | ❌ No input → All LEDs red                |
| 1 | 0 | 0 | ✅ A valid → A green, others ignored      |

---

## ✅ Final Test 5: Unlock Pattern – State Transition / Timeout Test

#### Scenario: System transitions through unlock states based on input sequence

```gherkin
Given the system is idle and ready to begin the unlock test
When the user presses buttons within a 5-second window
And the input sequence matches A → B → C → B → A
Then the system transitions through INIT, STEP_1, STEP_2, STEP_3, and STEP_4
And the system enters the PASS state after the final input
And the LED feedback reflects each state transition
```

### 🔖 Type

State Transition with Timeout and Recovery Path

### 🧭 Objective

Simulate a system requiring a precise unlock pattern:  
`A → B → C → B → A`  
Wrong input or delay triggers timeout. Recovery: `C x2`

### 🛠️ Setup

- **Time Limit**: 5 seconds  
- **Sequence**:
  1. A → INIT  
  2. B → STEP 1  
  3. C → STEP 2  
  4. B → STEP 3  
  5. A → PASS  
- Timeout or incorrect input → TIMEOUT  
- Recovery: C x2 → RECOVERY

### 🔧 State Diagram

```plaintext
IDLE
  ↓ (Start Test)
INIT
  ↓ A
STEP_1
  ↓ B
STEP_2
  ↓ C
STEP_3
  ↓ B
STEP_4
  ↓ A → PASS
  ↓ Timeout or wrong input → TIMEOUT
TIMEOUT
  ↓ C x2 → RECOVERY
  ↓ Timeout or no input → FAIL
PASS / RECOVERY / FAIL → IDLE
```



