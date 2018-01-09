#**PID Control** 

**PID Control Project**

The goals / steps of this project are the following:
* Write a simple PID controller
* Fine-tune the parameters
* Get the car to circle the loop at least once.
* And report on my understanding of PID and the process followed 
** This here markdown readme is the written report!


[//]: # (Image References)
[image1]: ./High_P.png "Way too high of a P value"
[image2]: ./Working_PD.png "Working P and D controls"

##Here I will consider the [rubric points](https://review.udacity.com/#!/rubrics/824/view) individually and describe how I addressed each point in my implementation.  

---

###1. Your code should compile.

I am fairly certain my code compiles and runs without exceptions.  For some reason the onDisconnection code had a 
seg-fault exception when it tried to close the uWS, which may be specific to the platform I am on and was commented out. 

Although I would prefer to avoid a "works on my machine", I tried just now for a few minutes to setup a 
Travis CI-build for the repo.  Seems like it will take some configuration and setup, but something exciting to consider 
for future projects.

###2. The PID procedure follows what was taught in the lessons
All the code I used, with the exception of a smoothing parameter for I-error, is taken from Sebastian Thrun's lectures
on PID controller and translated into equivalent or similar code in C++.  This includes the Twiddle code.  

###3. Describe the effect each of the P, I, D components had in your implementation.
In summary, the P "Proportional" component controls how strongly the controller reacts to the immediate error being provided.
Within the context of this project, P is used to control the steering and thus a strong error with a higher P value
will result in a higher steering angle being applied to the car.

The D component is short for "Derivative" and controls how strongly the car should react to changes in error.  
This component is necessary because with P alone, the car will merely keep steering without trying to reduce more than
necessary from the P component which will lead to overshooting/oversteering and the car falling off the road.  Thus,
this component in effect tries to slow down the turn radius as we approach the optimal position (in the center of the road).   

The I component is short for "Integral" and is a summation over the sum of all the previous errors combined.  This helps
to alleviate systematic biases in the system, such as if the car's wheels are not aligned and causes the car to drift.


The P and D components ended up being the only parts I included in order to complete the project.  It seems that
most people had similar conclusions.  The reasoning behind this is explained in the next section.


###4. Describe how the final hyperparameters were chosen.


Let's start with the I error and the main problem with it, being that it sums up to really large values.  Because I've had the fortune to work previously
on math-heavy projects with some talented folks, I have learned about concepts that I would call "alpha-smoothing", which
was one of the first ideas I tried to implement to alleviate the effects of I becoming too large.  The idea behind alpha-
smoothing is to give a stronger weight to more recent errors over older errors.  This is very useful in the context
of our project where the car will run around the entire track.  How poorly it did on one turn shouldn't have too strong of
an effect the next time it takes a different turn.

In general, I played for a long while manually with values and couldn't get the car to get much further than past the bridge.
One thing apparently obvious is the parameters had to be small, as our values need to be between -1 and 1 anyway, here
is a picture of what happens when the P-component is set really high:
 ![alt text][image1]



I spent a good while to get the Twiddle "algorithm" to work in the context of the project using Thrun's lecture.  Although
the code is not as clear as I would like it as it was hard to separate the "run" code from the "Twiddle" code, a lot of
long if-else and state variables had to be kept.  I alleviated this as best as I could by compartmentalizing the code
into clearly-named methods that explain what's going on (my favorite form of writing code and avoiding comments, because the compiler
enforces things to stay that way and reminds the code author what the method is doing just by typing its name).

Being my first usage of Twiddle and how highly Sebastian spoke of it, I was not too impressed.  The "best" parameters
selected by Twiddle, even after running for hours, were heavily weighted towards what was found in the beginning.  Maybe
the probe initial values were not the right ones to use, but manually tuning was more effective in this case than Twiddle.

In the end, I ended up dramatically slowing down the vehicle to help get it over the finish line since I need to move
on to the more challenging Project 5! 


And finally, here's an example of what the car should look like under optimal conditions:

 ![alt text][image2]


###5. The vehicle must successfully drive a lap around the track.

I believe you'll have to verify for yourself, but I would trust myself in the car the way it drove and it
successfully made it around the lap per my observations. 