# DEVELOPMENT
## Blog 1 - Goals
I started this project as a fun way to get into USB HID peripheral design. Two days of development have now passed, and I have realised I have not formally set out any project goals; so here they are:
### Ergonomics
Undoubtedly some of the biggest factors in building a custom mouse are its ergonomics. Whether you have a large (or small) hand size, weird grip or other needs, it can be incredibly useful to build and fine tune
a mouse to your exact needs. Whether its by changing the weight, shell type or handedness of the mouse, a mouse matched to your hand can make a world of difference. Personally, I wanted to try how a sub 20g mouse feels,
so there is my first goal.\
\
**Goal 1 - Total mass less than 20g exluding cable**

![image](https://github.com/user-attachments/assets/eb0a7802-f56d-4db3-bb17-41789102c773)\
*Very early (non-frame) case design where I was testing button spacing and total mouse length*

### Customisation
Designing my own mouse, I am able to add (and remove) whatever features I wish. For example, I like the feature of being able to cycle through DPI stages directly on my mouse without the need for external software, for this reason I implemented seamless DPI changing and storage on EEPROM. I also decided to not incorporate a scroll wheel into my first
prototype, as it is largely unused in modern games and would only complicate the design process.

**Goal 2 - DPI selection available on the mouse without the need for additional software**

```cpp
while (button_pressed(21) == 0){
  if (button_pressed(22) == 1){
    dpi_set ++; 
    dpi_set %= 4; //Dpi_set operated on by modulus to reduce size
    dpi_current = dpi_array[dpi_set]; //Dpi chosen
  }

  //Additional debounce to ensure switch has been released before cycling
  while(button_pressed(22) == 0){
    delay(10);
  } 
}
//Additional debounce to ensure switch has been released before saving
while (button_pressed(21) == 1){
  delay(10);
}
```
*DPI select code snippet*

### Response Time
It's all good if the mouse is ergonomic and has cool features but if it lacks the very essence of a mouse designed for competition what is the point. This is why I have focused on achieveing a ~1000Hz polling rate and minimal click latency. For instance, I chose a polling based approach (instead of interrupt based) for the optical sensor so that a defined polling rate could be consistently met. Furthermore, I designed a 1ms software debouncing algorithm that deals with both bouncing and chatter. The click sensor poll rate is not quite 1000Hz (~889Hz) however the difference cannot be perceived by a human and alteration to decrease (to get to the 1000hz) or increase (to improve reliability) the click polling rate is always possible in the firmware is always possible.
