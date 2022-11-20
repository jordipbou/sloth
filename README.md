# SLOTH

An ANS Forth implemented on top of CAPYBARA VM.

## Ideas

* Return to C on last call can be implemented in eval function, by pushing as
return address the location of code that returns to C, without needing to check
everytime if return stack has underflow (this may be independent to manage 
underflow error).


