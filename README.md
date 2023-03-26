# Rendering Atmospheric Scattering with Volumetric Path Tracing
CSE272 WI23 Final project, Leo Cao
## Introduction

In this project, I built upon the volumetric path tracer we implemented in LaJolla
to render realistic atmospheric effects.  

I used physically based models and parameters for the earth, 
the sun, and the atmosphere. My implementation also accounts for multiple scattering and global illumination, which is important 
for physical rendering of sunset, sunrise, and the shadow of the Earth inside the atmosphere (the Belt of Venus effect).

## Physical Model and Parameters

  ### Astronomical Model
  I modeled every object in space to-scale.  
  The earth is modeled as a lambertian sphere with radius $Re = 6378 km$. [1]
  If we are considering multiple scattering, the ground reflection of the sunlight into the atmosphere will actually make a difference in 
  the color of the sky. As discussed in papers like Nishita Et al.'s "Display method of the sky color taking into account multiple scattering".
  
  
  
  For the radius of the Sun, I used 

  ### Atmospheric Model
  
## References
[1]: <https://en.wikipedia.org/wiki/Earth_radius>
wikipedia for earth radius: https://en.wikipedia.org/wiki/Earth_radius  

