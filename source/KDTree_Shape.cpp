#include "KDTree_shape.hpp"

KDTree_shape::KDTree_shape() {

}


std::shared_ptr<KDTree_shape> KDTree_shape::build(std::vector<std::shared_ptr<Element > > & elements,  int depth, bool verbose) {

	// Creating the node
	std::shared_ptr<KDTree_shape> node = std::make_shared<KDTree_shape>( KDTree_shape() );
	node -> elements = elements;
	node -> left = nullptr;
	node -> right = nullptr;
	node -> set_depth(depth);

	node -> bbox = BBox();

	if (elements.size() == 0) {
		if (verbose) {
			std::cout << "Empty node" << std::endl;
			std::cout << "Leaf depth: " << depth << std::endl;
		}
		return node;
	}

	// If the node only contains one triangle,
	// there's no point in subdividing it more
	if (elements.size() == 1) {

		node -> bbox . update(elements[0]);

		node -> left = std::make_shared<KDTree_shape>( KDTree_shape() );
		node -> right = std::make_shared<KDTree_shape>( KDTree_shape() );

		node -> left -> elements = std::vector<std::shared_ptr<Element> >();
		node -> right -> elements = std::vector<std::shared_ptr<Element> >();


		return node;

	}

	node -> bbox.update(elements);


	arma::vec midpoint = arma::zeros<arma::vec>(3);

	// Could multithread here
	for (unsigned int i = 0; i < elements.size(); ++i) {

		// The midpoint of all the elements is found





		midpoint += (*dynamic_cast<Facet *>(elements[i].get()) -> get_facet_center()) * (1. / elements.size());
	}

	// Facets to be assigned to the left and right nodes
	std::vector < std::shared_ptr<Element> > left_facets;
	std::vector < std::shared_ptr<Element> > right_facets;

	unsigned int longest_axis = node -> bbox.get_longest_axis();

	for (unsigned int i = 0; i < elements.size() ; ++i) {

		bool added_to_left = false;
		bool added_to_right = false;

		for (unsigned int v = 0; v < 3; ++v) {

			// The elements currently owned by the node are split
			// based on where their vertices lie

			if ( midpoint(longest_axis) >= elements[i] -> get_control_points() -> at(v) -> get_coordinates() -> at(longest_axis)
			        && added_to_left == false) {
				left_facets.push_back(elements[i]);
				added_to_left = true;
			}

			else if (midpoint(longest_axis) <= elements[i] -> get_control_points() -> at(v) -> get_coordinates() -> at(longest_axis)
			         && added_to_right == false) {
				right_facets.push_back(elements[i]);
				added_to_right = true;
			}

		}

	}

	// I guess this could be avoided
	if (left_facets.size() == 0 && right_facets.size() > 0) {
		left_facets = right_facets;
	}

	if (right_facets.size() == 0 && left_facets.size() > 0) {
		right_facets = left_facets;
	}

	unsigned int matches = 0;

	for (unsigned int i = 0; i < left_facets.size(); ++i) {
		for (unsigned int j = 0; j < right_facets.size(); ++j) {
			if (left_facets[i] == right_facets[j]) {
				++matches;
			}
		}
	}



	// Subdivision stops if at least 50% of triangles are shared amongst the two leaves
	// or if this node has reached the maximum depth
	// specified in KDTree_shape.hpp (1000 by default)
	if ((double)matches / left_facets.size() < 0.5 && (double)matches / right_facets.size() < 0.5 && depth < this -> max_depth) {


		// Recursion continues
		node -> left = build(left_facets, depth + 1, verbose);
		node -> right = build(right_facets, depth + 1, verbose);

	}

	else {

		node -> left = std::make_shared<KDTree_shape>( KDTree_shape() );
		node -> right = std::make_shared<KDTree_shape>( KDTree_shape() );

		node -> left -> elements = std::vector<std::shared_ptr<Element> >();
		node -> right -> elements = std::vector<std::shared_ptr<Element> >();

		if (verbose) {

			std::cout << "Leaf depth: " << depth << std::endl;
			std::cout << "Leaf contains: " << node -> elements.size() << " elements " << std::endl;

			node -> bbox.print();
			// Uncomment if willing to save the leaf bounding boxes to a
			// readable obj file
			std::string path = std::to_string(rand() ) + ".obj";
			node -> bbox.save_to_file(path);

		}

	}

	return node;

}


bool KDTree_shape::hit(KDTree_shape * node, Ray * ray, bool computed_mes) const {

	// Check if the ray intersects the bounding box of the given node

	if (node -> hit_bbox(ray, computed_mes)) {

		bool hit_facet = false;

		// If there are triangles in the child leaves, those are checked
		// for intersect. First, the method checks whether it is still on a branch
		if (node -> left -> elements.size() > 0 || node -> right -> elements.size() > 0) {

			bool hitleft = this -> hit(node -> left.get(), ray, computed_mes);
			bool hitright = this -> hit(node -> right.get(), ray, computed_mes);

			return (hitleft || hitright);

		}

		else {

			// If not, the current node is a leaf
			for (unsigned int i = 0; i < node -> elements.size(); ++i) {

				// If there is a hit
				if (ray -> single_facet_ray_casting( dynamic_cast<Facet * >(node -> elements[i].get()), computed_mes)) {
					hit_facet = true;
				}

			}

			if (hit_facet) {
				return true;
			}

			return false;

		}
	}
	return false;

}

void KDTree_shape::set_depth(int depth) {
	this -> depth = depth;
}



bool KDTree_shape::hit_bbox(Ray * ray, bool computed_mes) const {

	arma::vec * u = ray -> get_direction_target_frame();
	arma::vec * origin = ray -> get_origin_target_frame();


	arma::vec all_t(6);

	all_t(0) = (this ->bbox . get_xmin() - origin -> at(0)) / u -> at(0);
	all_t(1) = (this ->bbox . get_xmax() - origin -> at(0)) / u -> at(0);

	all_t(2) = (this ->bbox . get_ymin() - origin -> at(1)) / u -> at(1);
	all_t(3) = (this ->bbox . get_ymax() - origin -> at(1)) / u -> at(1);

	all_t(4) = (this ->bbox . get_zmin() - origin -> at(2)) / u -> at(2);
	all_t(5) = (this ->bbox . get_zmax() - origin -> at(2)) / u -> at(2);

	arma::vec all_t_sorted = arma::sort(all_t);

	double t_test = 0.5 * (all_t_sorted(2) + all_t_sorted(3));

	arma::vec test_point = *origin + t_test * (*u);

	// If the current minimum range for this Ray is less than the distance to this bounding box,
	// this bounding box is ignored
	if (computed_mes) {
		if (ray -> get_computed_range() < all_t_sorted(2)) {
			return false;
		}
	}
	else {
		if (ray -> get_true_range() < all_t_sorted(2)) {
			return false;
		}
	}

	if (test_point(0) <= this -> bbox . get_xmax() && test_point(0) >= this -> bbox . get_xmin()) {

		if (test_point(1) <= this -> bbox . get_ymax() && test_point(1) >= this -> bbox  .get_ymin()) {

			if (test_point(2) <= this -> bbox . get_zmax() && test_point(2) >= this -> bbox . get_zmin()) {


				return true;

			}
		}
	}

	return false;


}





