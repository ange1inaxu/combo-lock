[demonstration video](https://youtu.be/b1zzTXdVgA4)

Because we are only turning our Arduino clockwise or counterclockwise on a level table, we only needed the z-axis angular velocity reading from the gyroscope. Setting LOOP_PERIOD to 10 ms, we know that our gyroscope reading occurs every 10 ms, which we can use to integrate our angular velocity to get our angular position: `angle = angle + 0.001*LOOP_PERIOD*velocity;` In addition, we made sure that the velocity reading was greater than 1 or less than -1 to make sure that the tilt was intentional, rather than noise in the gyroscope reading. Since the displacement could be negative if tilted Clockwise or greater than 360 if turned several times Counterclockwise, we normalized our angle, so that it'd always be between 0 and 360. Since we have 10 digits, a change in digit value would correspond with a change in 36 degrees, so using some arithmetic, we're able convert the angular position to a digit. This is done in our helper function get_digit().

From here, we used a finite state machine lock_combo_fsm(). We start in the state=REST_UNLOCKED, where we print directions to the user: "UNLOCKED Please press button 1 to enter a combo." Inside this state, we have another FSM in place to track whether button1 is pressed or not (variable button1_pressed is 1 or 0, respectively) and program_mode is initialized as 1. If button1 is not pressed and program_mode=1, we print a reminder to the user to turn CW, and we repeatedly call on get_digit() and display it to the TFT screen so the user can see the changing digit, corresponding to the angular position. Once button1 is pressed, we insert digit into the first index of our combo array, which stores our three-digit passcode. Once button1 is released, we increment to program_mode=2 to insert the 2nd digit using the same procedure, along with the 3rd digit in program_mode=3.

Once all 3 digits in our passcode are determined, we have another FSM to track whether button2 is pressed or not (variable button2_pressed is 1 or 0, respectively). If it's pressed and released, we shift state=LOCKED and change our background color from blue to red.

In this state, we check again if button1 is pressed then released, thereby entering state=ENTRY_MODE1 for the user to enter the passcode's first digit. Using a similar procedure to program_mode, we repeatedly call on get_digit() and display it to the TFT screen so the user can see the changing digit. Once they press button1, we enter state=CHECK1 where we check if the user's digit matches combo's first digit. If it doesn't, we immediately return to state=LOCKED. Otherwise, we proceed to the 2nd digit entry with state=ENTRY_MODE2. The same procedure is done to enter digits 2 and 3. At the final check, state=CHECK3, if all the digits match, then we revert state=REST_UNLOCKED, paint our TFT screen green, print an inspirational quote, and reset all our FSM state variables so that another button1 press restarts the process.