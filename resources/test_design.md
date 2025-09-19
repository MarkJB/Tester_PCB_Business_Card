# 🧪 Test Suite Overview

| Test Case # | Name                       | Type                                      |
| ----------- | -------------------------- | ----------------------------------------- |
| 1           | Button Functionality Check | Smoke / Sanity Test                       |
| 2           | Input Debounce Validation  | Boundary Value / Timing Test              |
| 3           | Conditional Logic          | Decision Table / Branch Test              |
| 4           | Input Range Check          | Equivalence Partitioning / Threshold Test |
| 5           | Unlock Pattern             | State Transition / Timeout Test           |

### 💡 Test Result Feedback

| Outcome   | LED Behavior               |
| --------- | -------------------------- |
| ✅Pass    | 🟢⚫                       |
| ⚠️Warning | (🟢/⚫)⚫ (Blinking Green) |
| ❌Fail    | ⚫🔴                       |

## 🚨⚠️ Warning! Spoilers Ahead!! ⚠️🚨

We should split the test description and the steps/solutions into separate files so this can serve as a test guide without giving away the solution

## 🧪 Test Case 1: Button Functionality Check

**Instruction**: Press each button in turn.  
**Type**: Smoke / Sanity Test

#### Scenario: Validate input button functionality

```gherkin
Given the system state is powered on and in an idle state
When I press each input button in sequence
Then each button press is registered by the system
And the system is ready for further testing
```

---

## 🧪 Test Case 2: Input Debounce Validation – Boundary Value / Timing Test

> ### Note: This needs some clarification of what we are trying to achieve - what are we testing - that it works or rejects?

**Instruction**: Press button B twice with a specific delay between presses.
**Type**: Boundary Value / Timing Test

### Scenario: Validate system debounce threshold using timed input

```gherkin
Given the system is idle and ready to receive input
When I press button B twice with a delay greater than 30ms and less than 200ms
Then the system registers both presses as valid input

Given the system is idle and ready to receive input
When I press button B twice with a delay less than 30ms
Then the system ignores the second press as invalid input (debounce)
```

### 🧠 What’s Being Tested

> “This test models a system that filters out rapid, repeated signals. The tester must press B twice with a precise delay to determine the system’s debounce threshold.”

- **Boundary**: 100ms (tbd)
- **Pass**: Two presses >30ms apart + <200ms apart → both counted
- **Fail**: Two presses <30ms apart → second ignored

### 🔧 Test Logic

| Timing Between Presses | Interpretation              | Test Outcome |
| ---------------------- | --------------------------- | ------------ |
| >100ms                 | Treated as valid input      | ✅ Pass      |
| <100ms                 | Treated as bounce → ignored | ❌ Fail      |

---

## 🧪 Test Case 3: Conditional Logic – Decision Table Test

**Instruction**: Press any combination of A, B, and C within 5 seconds.
**Type**: Decision Table / Branch Test

### Scenario: Evaluate system logic based on input combinations

```gherkin
Given the system is idle and ready to receive input
When the user presses any combination of buttons (individually or combined) A, B, and C within a 5-second window
Then the system stores the input state until the timeout expires
And the system evaluates the final input state against its decision table
```

> Note: Pressing a button will set its state and pressing it again will unset it.

### 🧠 What’s Being Tested

> “This test models decision table logic. The tester must deduce the correct input combination by observing system outcomes and reasoning through partial correctness.”

- **Inputs**: A, B, C
- **Time Limit**: 5 seconds
- **Outcome Types**:
  - ✅ **Pass** → Correct combination
  - ⚠️ **Warning** → Partial match
  - ❌ **Fail** → Incorrect or no input

### 🔧 Test Logic

| A   | B   | C   | Outcome | Reasoning                           |
| --- | --- | --- | ------- | ----------------------------------- |
| 0   | 0   | 0   | Fail    | No input → no correct signal        |
| 0   | 0   | 1   | Warning | C alone → partial match             |
| 0   | 1   | 0   | Warning | B alone → partial match             |
| 0   | 1   | 1   | Pass    | ✅ B + C → correct combo            |
| 1   | 0   | 0   | Fail    | A alone → incorrect input           |
| 1   | 0   | 1   | Warning | A + C → C is good, A is not         |
| 1   | 1   | 0   | Warning | A + B → B is good, A is not         |
| 1   | 1   | 1   | Warning | B + C is good, but A degrades combo |

---

## 🧪 Test Case 4: Input Range Check – Equivalence Partitioning / Threshold Test

**Instruction**: Provide values for A, B, and/or C within a 5-second window.
**Type**: Equivalence Partitioning / Threshold Test

### Scenario: System validates input ranges after timeout

```gherkin
Given the system is idle and ready to receive input
When the user provides values for inputs A, B, and/or C within a 5-second window
Then the system stores the final input state at the end of the time window
And the system evaluates each input independently against its valid range
```

### 🧠 What’s Being Tested

> “This test models per-input validation using equivalence partitioning. Each input is evaluated independently against its valid range. It teaches testers to think in terms of input classes and boundary sensitivity.”

### 📊 Example Outcomes

| A      | B       | C      | Outcome Description                                                                                                                                  |
| ------ | ------- | ------ | ---------------------------------------------------------------------------------------------------------------------------------------------------- |
| 6      | 8-11    | 3      | ✅ All valid → pass                                                                                                                                  |
| 5 or 7 | 7 or 12 | 2 or 4 | ⚠️ borderline → warning (red flashing - I originally intended to use bicolour LEDs which could produce Green/Red/Orange but they were too expensive) |
| 0      | 2       | 0      | ❌ B invalid → fail                                                                                                                                  |
| 0      | 0       | 0      | ❌ No input → fail                                                                                                                                   |
| 7      | 0       | 0      | ✅ A valid → pass, others ignored                                                                                                                    |
| 0      | 8       | 0      | ✅ B valid → pass, others ignored                                                                                                                    |
| 0      | 0       | 3      | ✅ C valid → pass, others ignored                                                                                                                    |

---

## 🧪 Test Case 5: Unlock Pattern – State Transition / Timeout + Recovery

**Instruction**: Enter the correct unlock code within the time limit.  
**Type**: State Transition, Timeout Handling

---

### 📜 Scenario: System transitions through unlock states based on input sequence

```gherkin
Scenario: Successful unlock
  Given the unlock test is in progress
  When the user enters the sequence A → B → C → B → A
  Then the system transitions through INIT, STEP_1, STEP_2, STEP_3, and STEP_4
  And the system enters the PASS state
  And the system is unlocked

Scenario: Incorrect unlock sequence
  Given the unlock test is in progress
  When the user enters any 5-button sequence that does not match A → B → C → B → A
  Then the system enters the RECOVERY state
  And the test case LEDs stop alternating
  And the FAIL LED blinks rapidly
  And the user has 5 seconds to reset the test by pressing C → C

Scenario: Recovery success
  Given the system is in the RECOVERY state
  When the user presses C → C within 5 seconds
  Then the RECOVERY timeout is cancelled
  And the FAIL LED is cleared
  And the test case LEDs resume alternating
  And the user may retry the unlock sequence

Scenario: Recovery failure
  Given the system is in the RECOVERY state
  When the user does not press C → C within 5 seconds
  Then the system enters the FAIL state
  And the FAIL LED remains solid
```

### 🧭 Objective

Simulate a system requiring a precise unlock pattern:  
`A → B → C → B → A`

- Input must be exactly 5 button presses
- Incorrect sequence triggers recovery opportunity
- Recovery: `C → C` within 5 seconds

---

### 🛠️ Setup

- **Test Duration**: 10 seconds total from first input
- **Input Limit**: Exactly 5 button presses
- **Correct Sequence**:
  1. A → INIT
  2. B → STEP_1
  3. C → STEP_2
  4. B → STEP_3
  5. A → PASS
- **Incorrect Sequence**: Any 5-button input that does not match the above
- **Recovery Trigger**: Only after incorrect 5-button input
- **Recovery Window**: 5 seconds to enter `C → C`
- **Post-Recovery**: Test resets, user may retry

---

### 💡 LED Feedback

| State       | LED Behavior                                 |
| ----------- | -------------------------------------------- |
| In Progress | Alternating red/green blink (test case LEDs) |
| PASS        | Solid green                                  |
| FAIL        | Solid red                                    |
| RECOVERY    | Rapid red blink (FAIL LED)                   |

> Note: Individual button validity is not shown. INIT LED pulses on button press via event handler.

---
