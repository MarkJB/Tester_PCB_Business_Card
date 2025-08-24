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
When I press button B twice with a delay greater than 100ms
Then the system registers both presses as valid input

Given the system is idle and ready to receive input
When I press button B twice with a delay less than 100ms
Then the system ignores the second press as invalid input
```

### 🧠 What’s Being Tested

> “This test models a system that filters out rapid, repeated signals. The tester must press B twice with a precise delay to determine the system’s debounce threshold.”

- **Boundary**: 100ms (tbd)
- **Pass**: Two presses >100ms apart → both counted
- **Fail**: Two presses <100ms apart → second ignored

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

| A   | B   | C   | Outcome Description                   |
| --- | --- | --- | ------------------------------------- |
| 2   | 6   | 3   | ✅ All valid → Green LEDs for A/B/C   |
| 3   | 6   | 3   | ⚠️ A borderline → Result blinks green |
| 0   | 2   | 0   | ❌ B invalid → B red                  |
| 0   | 0   | 0   | ❌ No input → All LEDs red            |
| 1   | 0   | 0   | ✅ A valid → A green, others ignored  |

---

## 🧪 Test Case 5: Unlock Pattern – State Transition / Timeout Test

**Instruction**: Provide unlock code before input times out.
**Type**: Equivalence Partitioning / Threshold Test

#### Scenario: System transitions through unlock states based on input sequence

```gherkin
Given the the unlock test begins
When the user presses buttons within the test time window (10 seconds)
And the input sequence matches A → B → C → B → A
Then the system transitions through INIT, STEP_1, STEP_2, STEP_3, and STEP_4
And the system enters the PASS state after the final input
And the system is 'unlocked'

Given the system is ready to begin the unlock test
When the user enters an incorrect button unlock sequence (same number of button presses as the correct code)
Then the system enters a RECOVERY state
And the current test case LEDs stop blinking alternately
And the current test case FAIL LEDs blinks rapidly
And the user has a 5 second time to reset the count by pressing the c button twice

Given the system is in the RECOVERY state
When the user clears the RECOVERY state by pressing the c button twice
Then the RECOVERY timeout is cancelled and the test timeout is reset
And the rapidly blinking FAIL LED is cleared
And the current test case LEDs start blinking alternatley
And the user can make another attempt at entering the correct code.
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
