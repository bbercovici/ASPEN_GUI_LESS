
#ifndef HEADER_SHAPEMODELTRI
#define HEADER_SHAPEMODELTRI

#include <string>
#include <string>
#include <iostream>
#include <armadillo>
#include <set>
#include <map>
#include <limits>

#include "ShapeModel.hpp"
#include "Facet.hpp"
#include "ControlPoint.hpp"
#include "KDTree_shape.hpp"

 class KDTree_shape;


/**
Declaration of the ShapeModel class. Specialized
implementation storing an explicit facet/vertex shape model
*/
class ShapeModelTri : public ShapeModel {

public:

	
	/**
	Constructor
	@param frame_graph Pointer to the graph storing
	reference frame relationships
	@param frame_graph Pointer to the reference frame graph
	*/
	ShapeModelTri(std::string ref_frame_name,
	           FrameGraph * frame_graph) : ShapeModel(ref_frame_name,frame_graph){};


	/**
	Constructs the KDTree holding the shape model
	@param verbose true will save the bounding boxes to a file and display
	kd tree construction details
	*/
	void construct_kd_tree(bool verbose = false);


	/**
	Determines whether the provided point lies inside or outside the shape model.
	The shape model must have a closed surface for this method to be trusted
	@param point coordinates of the point to be tested expressed in the shape model frame
	@param tol numerical tolerance ,i.e value under which the lagrangian of the "surface field"
		below which the point is considered outside
	@return true if point is contained inside the shape, false otherwise
	*/
	bool contains(double * point, double tol = 1e-6) ;


	/**
	Returns pointer to KDTree member.
	@return pointer to KDtree
	*/
	std::shared_ptr<KDTree_shape> get_kdtree() const ;

	/**
	Checks that the normals were consistently oriented. If not,
	the ordering of the vertices in the provided shape model file is incorrect
	@param tol numerical tolerance (if consistent: norm(Sum(oriented_surface_area)) / average_facet_surface_area << tol)
	*/
	void check_normals_consistency(double tol = 1e-3) const;


	/**
	Augment the internal container storing facets with a new (and not already inserted)
	one
	@param facet pointer to the new facet to be inserted
	*/
	void add_facet(std::shared_ptr<Facet> facet);

	/**
	Augment the internal container storing vertices with a new (and not already inserted)
	one
	@param vertex pointer to the new vertex to be inserted
	*/
	void add_vertex(std::shared_ptr<ControlPoint> vertex);

	/**
	Defines the reference frame attached to the shape model
	@param ref_frame Pointer to the reference frame attached
	to the shape model
	*/
	void set_ref_frame_name(std::string ref_frame_name);

	


	/**
	Returns the dimensions of the bounding box
	@param Bounding box dimension to be computed (xmin,ymin,zmin,xmax,ymax,zmax)
	*/
	void get_bounding_box(double * bounding_box) const;

	/**
	Saves the shape model in the form of an .obj file
	@param path Location of the saved file
	*/
	void save(std::string path) const;


	/**
	Returns the surface area of the shape model
	@return surface area (U^2 where U is the unit of the shape coordinates)
	*/
	double get_surface_area() const;

	/**
	Returns the volume of the provided shape model
	@return volume (U^2 where U is the unit of the shape coordinates)
	*/
	double get_volume() const;

	/**
	Returns the location of the center of mass
	@return pointer to center of mass
	*/
	arma::vec * get_center_of_mass();

	/**
	Returns the name of the reference frame attached to this
	ref frame
	@return name of reference frame
	*/
	std::string get_ref_frame_name() const;

	/**
	Splits the provided facets in four facets.
				V4 --------------V0--------------V3
				 \  \	  F7    /  \   F1    /   /
				  \	   \	  /	    \      /    /
				   \	  \  /   F0   \  /     /
				    \	F6  V8-------V7  F2   /
				     \	   /  \	 F5  / \     /
				      \   /    \    /   \   /
	                   \ /  F4  \  /  F3 \ /
						V1-------V6-------V2
						 \        |       /
						  \   F9  | F8   /
						   \      |     /
							\     |    /
							 \    |   /
							  \   |  /
							   \  | /
	                            \ |/
	                             V5
	Adds
		- 10 facets
		- 3 vertices
		- 6 edges
	Removes
		- 3 facets
		- 3 edges
	@param facet Pointer to facet to be split. THIS POINTER
	WILL BECOME INVALID AFTER THE FACET IS SPLIT
	@param seen_facets set containing the facets that were in view of the filter before recycling took place.
	This set will be edited to ensure that the facets that remain are all valid
	*/
	void split_facet(Facet * facet, std::set<Facet *> & seen_facets);




	/**
	Removes this facet from the shape model by merging together the two vertices
	on the edge facing the smallest angle in the facet
	@param minimum_angle If the smallest angle protacted by 2 of the facet's 
	@param facet Pointer to the facet to recycle. THIS POINTER
	WILL BECOME INVALID AFTER THE FACET IS SPLIT
	@param seen_facets set containing the facets that were in view of the filter before recycling took place.
	This set will be edited to ensure that the facets that remain are all valid
	@param spurious_facets set containing facets that must be removed from the shape model
	because they are flipped
	@return true if the facet was recycled, false otherwise
	*/
	bool merge_shrunk_facet(double minimum_angle,
	                        Facet * facet,
	                        std::set<Facet *> * seen_facets,
	                        std::set<Facet *> * spurious_facets = nullptr);


	/**
	Updates the values of the center of mass, volume, surface area
	*/
	void update_mass_properties();

	/**
	Update all the facets of the shape model
	*/
	void update_facets() ;



	/**
	Updates the specified facets of the shape model. Ensures consistency between the vertex coordinates
	and the facet surface area, normals and centers.
	@param facets Facets to be updated
	@param compute_dyad true if the facet dyad needs to be computed/updated
	*/
	void update_facets(std::set<Facet *> & facets);


	/**
	Shifts the coordinates of the shape model
	so as to have (0,0,0) aligned with its barycenter
	The resulting barycenter coordinates are (0,0,0)
	*/
	void shift_to_barycenter();

	/**
	Applies a rotation that aligns the body
	with its principal axes.
	This assumes that the body has been shifted so
	that (0,0,0) lies at its barycenter
	The resulting inertia tensor is diagonal
	Undefined behavior if
	the inertia tensor has not been computed beforehand
	*/
	void align_with_principal_axes();

	/**
	Returns the non-dimensional inertia tensor of the body in the body-fixed
	principal axes. (rho == 1, l = (volume)^(1/3))
	@return principal inertia tensor
	*/
	arma::mat get_inertia() const;


	/**
	Computes the surface area of the shape model
	*/
	virtual void compute_surface_area();
	/**
	Computes the volume of the shape model
	*/
	virtual void compute_volume();
	/**
	Computes the center of mass of the shape model
	*/
	virtual void compute_center_of_mass();
	/**
	Computes the inertia tensor of the shape model
	*/
	virtual void compute_inertia();


	/**
	Finds the intersect between the provided ray and the shape model
	@param ray pointer to ray. If a hit is found, the ray's internal is changed to store the range to the hit point
	@param computed_mes true if the target is the estimated shape
	*/
	virtual bool ray_trace(Ray * ray,bool computed_mes);


protected:
	
	double volume;
	double surface_area;

	arma::vec cm;

	arma::mat inertia;


	std::shared_ptr<KDTree_shape> kd_tree = nullptr;



};

#endif