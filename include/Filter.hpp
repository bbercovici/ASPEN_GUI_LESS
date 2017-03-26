#ifndef HEADER_FILTER
#define HEADER_FILTER

#include "ShapeModel.hpp"
#include "Lidar.hpp"
#include "FrameGraph.hpp"



/**
Filter class hosting:
- the instrument
- the true shape model
- the estimated shape model
- Filtering tools:
# the partial derivatives evaluation
# shape refinement
*/
class Filter {

public:

	/**
	Constructor
	@param frame_graph Pointer to the graph storing the reference frames
	@param lidar Pointer to instrument
	@param true_shape_model Pointer to the true shape model
	@param estimated_shape_model Pointer to the estimated shape model
	@param t0 Initial time (s)
	@param t1 Final time (s)
	@param dt Timestep (s)
	*/
	Filter(FrameGraph * frame_graph,
	       Lidar * lidar,
	       ShapeModel * true_shape_model,
	       ShapeModel * estimated_shape_model,
	       double t0,
	       double tf,
	       double dt);

	void run();

protected:

	void step_in_time();
	void collect_observations();
	void compute_observations();
	double t0;
	double tf;
	double dt;

	FrameGraph * frame_graph;
	Lidar * lidar;
	ShapeModel * true_shape_model;
	ShapeModel * estimated_shape_model;



};


#endif