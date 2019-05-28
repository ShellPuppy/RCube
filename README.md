# RCube

Rubik's Cube solver for very large cubes.  

Solves even or odd size cubes of size >= 3

The primary limitation is the amount of memory on a system and the length of time it takes to solve.
Solve time grows N^3 which means it takes 8 times longer to solve a cube that is 2x larger. 

# Solve Method
Solves the centers then corners then edges

The center is solved in 15 stages where each stage moves all the pieces of a certain color from one face to the desired face. For example: a stage moves all green pieces on the white face to the green face. Repeat this for all colors and faces.

The solver uses a commutator that can move many pieces simultaneously which makes it about 4 times faster on average than a simple commutator that moves one piece at a time.  
Face rotations are free.  Instead of moving all the pieces on a given face to perform a rotation, the solver simply changes the coordinate system of the face. This saves an enormous amount of data swapping. 

The corners are solved using a basic brute force method of moving the corner into place and then rotating until the faces were oriented correctly. Will Smth can explain: https://www.youtube.com/watch?v=WBzkDrC9vQs
