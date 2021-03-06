

// U-shape
width       = 12.0;
height      = 16.0;
thickness   = 1.9;

// object size
o_width = 20;
o_height = 20;
o_thickness = 3;

// top cap
top_cap_od = 22.00;
top_cap_thick = 2.0;
top_cap_id = od - thick; 
top_cap_height = 20;
top_cap_r = 20;

top_cap(top_cap_r, top_cap_od, top_cap_id, top_cap_thick, top_cap_height, 200);

//test_base();
//u_shape(o_thickness);
//end_cap(20);
//center_joint(30);

module center_joint(r)
{
	difference()
	{
		difference()
		{
			difference()
			{
				difference()
				{
					difference()
					{
						// main sphere
						sphere(r, $fn=200);
						// cut out first leg
						rotate([60, 0, 0])
							translate([0,0,r-10])
								u_shape(r-11.0);
					}
					// cut out second leg
					rotate([60, 0, 120])
						translate([0,0,r-10])
							u_shape(r-11.0);
				}
				// cut third leg
				rotate([60, 0, 240])
					translate([0,0,r-10])
						u_shape(r-11.0);
	    		}
			// cut out top 
			rotate([180, 0, 0])
				translate([0,0,r-8])
					u_shape(r);
		}
        // cut plate for a flat bottom
		translate([0,0,41])
			cube([r * 2, r * 2, r], center=true);
    }
}

module test_base()
{
    difference()
    {
		cube([o_width, o_height, o_thickness], center=true);
        u_shape(o_thickness);
    }
}

module end_cap(r, res)
{
    difference()
	{
    		difference()
		{
    			sphere(r, $fn=res);
        		translate([0, 0, r]) 
        			u_shape(2 * r);
		}
        // cut plate
		translate([0,0,25])
			cube([r * 2, r * 2, r], center=true);
    }
}

module u_shape(t)
{
    difference()
    {
         cube([width + (thickness * 2), height, t], center=true);
         translate([0, thickness, 0])
             cube([width, height, t], center=true);
    }
} 

module top_cap(rad, od, id, thick, height, res)
{
	difference()
	{
		end_cap(rad, res);
		translate([0,0,10])
             cylinder(r = od / 2, h = height, $fn=res);
//    			difference()
//			{
//				cylinder(r = od / 2, h = height, $fn=res);
//    				cylinder(r = id / 2, h = height, $fn=res);
//			}
	}
}