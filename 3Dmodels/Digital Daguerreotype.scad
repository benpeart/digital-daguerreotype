face_plate_width = 203.2;
face_plate_height = 203.2;
face_plate_depth = 3;
screen_width = 97.6;
screen_height = 58.1;
camera_width = 99 - 2;  // the camera face is slightly smaller than the dimensions given by Intel
camera_height = 23 - 2;  // the camera face is slightly smaller than the dimensions given by Intel
camera_depth = 20.05;
camera_mount_width = 45;
screw_hole_diameter = 3;
screw_hole_offset = 8;

front_of_face_to_front_of_back = 29.56;

face_plate(face_plate_width, face_plate_height);

// move away from face plate
//translate([0, -3 * camera_height]) 

// move over face plate to check alignment of holes
//translate([(face_plate_width - camera_width) / 2, (face_plate_height / 4 * 3) - camera_height, face_plate_depth]) 
//    camera_bracket(camera_width, camera_height, camera_depth);

module face_plate(width, height)
{
    camera_origin = [(width - camera_width) / 2, (height / 4 * 3) - (camera_height / 2)];
    screen_origin = [(width - screen_width)/ 2, height / 2 - screen_height];
    
    linear_extrude(height=face_plate_depth) difference()
    {
        // make the faceplate
        square([width, height]);

        // make a hole for the camera
        translate(camera_origin) camera(camera_width, camera_height);
        
         // screw holes for camera bracket
        translate(camera_origin) translate([1.5 * screw_hole_diameter, 1.5 * screw_hole_diameter - camera_height / 2]) 
            circle(d = screw_hole_diameter, $fn=50);
        translate(camera_origin) translate([1.5 * screw_hole_diameter, camera_height - 1.5 * screw_hole_diameter + camera_height / 2]) 
            circle(d = screw_hole_diameter, $fn=50);
        translate(camera_origin) translate([camera_width - 1.5 * screw_hole_diameter, 1.5 * screw_hole_diameter - camera_height / 2]) 
            circle(d = screw_hole_diameter, $fn=50);
        translate(camera_origin) translate([camera_width - 1.5 * screw_hole_diameter, camera_height - 1.5 * screw_hole_diameter + camera_height / 2]) 
            circle(d = screw_hole_diameter, $fn=50);
        
        // make a hold for the screen
        translate(screen_origin) square([screen_width, screen_height]);
        
        // screw holes for screen
        translate(screen_origin) translate([-screw_hole_offset, -screw_hole_offset]) 
            circle(d = screw_hole_diameter, $fn=50);
        translate(screen_origin) translate([-screw_hole_offset, screen_height + screw_hole_offset]) 
            circle(d = screw_hole_diameter, $fn=50);
        translate(screen_origin) translate([screen_width + screw_hole_offset, -screw_hole_offset]) 
            circle(d = screw_hole_diameter, $fn=50);
        translate(screen_origin) translate([screen_width + screw_hole_offset, screen_height + screw_hole_offset]) 
            circle(d = screw_hole_diameter, $fn=50);
    }
}

module camera(width, height)
{
    translate([height / 2, height / 2]) union()
    {
        circle(d = height, $fn=50);
        translate([0, -height / 2]) square([width - height, height]);
        translate([width - height, 0]) circle(d = height, $fn=50);
    }
}

// dimensions should be for the camera size
// the walls will be expanded to leave at least this much interior room
module camera_bracket(width, height, depth) 
{
    // fudge factor to get render to show removed material correctly
    fudge = 0.01;

    wall_thickness = 3 * screw_hole_diameter;
    
    difference()
    {
        // make an opened ended bracket that will make the face of the camera flush with the face plate
        cube([width, 2 * height, depth]);
        translate([-fudge, wall_thickness, -fudge]) 
            cube([width + 2 * fudge, 2 * height - 2 * wall_thickness, depth - face_plate_depth + fudge]);
        translate([width / 9, -fudge, -fudge]) 
            cube([width * 7 / 9, 2 * (height + fudge), depth - face_plate_depth + fudge]);

         // screw holes for camera
        translate([width / 2 - camera_mount_width / 2, height]) 
            cylinder(d = screw_hole_diameter, h = height + fudge, $fn=50);
        translate([width / 2 + camera_mount_width / 2, height]) 
            cylinder(d = screw_hole_diameter, h = height + fudge, $fn=50);
        
         // screw holes for bracket (smaller to hold threads)
        translate([1.5 * screw_hole_diameter, 1.5 * screw_hole_diameter, -fudge]) 
            cylinder(d = screw_hole_diameter - 0.5, h = height + 2 * fudge, $fn=50);
        translate([1.5 * screw_hole_diameter, 2 * height - 1.5 * screw_hole_diameter, -fudge]) 
            cylinder(d = screw_hole_diameter - 0.5, h = height + 2 * fudge, $fn=50);
        translate([width - 1.5 * screw_hole_diameter, 1.5 * screw_hole_diameter, -fudge]) 
            cylinder(d = screw_hole_diameter - 0.5, h = height + 2 * fudge, $fn=50);
        translate([width - 1.5 * screw_hole_diameter, 2 * height - 1.5 * screw_hole_diameter, -fudge]) 
            cylinder(d = screw_hole_diameter - 0.5, h = height + 2 * fudge, $fn=50);
   }
}