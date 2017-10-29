#include "Wrappers.hpp"



arma::vec point_mass_dxdt_wrapper(double t, arma::vec X, Args * args) {

	

	arma::vec pos_inertial = X . rows(0, 2);
	arma::vec acc_inertial = args -> get_dyn_analyses() -> point_mass_acceleration(pos_inertial , args -> get_mass());

	arma::vec dxdt = { X(3), X(4), X(5), acc_inertial(0), acc_inertial(1), acc_inertial(2)};

	return dxdt;

}


arma::vec sigma_dot_wrapper(double t, arma::vec X, Args * args) {


	arma::vec omega = args -> get_constant_omega();
	arma::vec attitude_set = {X(0), X(1), X(2), omega(0), omega(1), omega(2)};

	arma::vec sigma_dot =  RBK::dmrpdt(t, attitude_set );
	return sigma_dot;
}





arma::vec point_mass_dxdt_wrapper_body_frame(double t, arma::vec X, Args * args) {

	
	arma::vec attitude_state = args -> get_interpolator() -> interpolate(t, true);

	arma::vec mrp_TN = attitude_state.rows(0, 2);
	arma::vec omega_TN = attitude_state.rows(3, 5);

	arma::vec pos_body = X . rows(0, 2);
	arma::vec vel_body = X . rows(3, 5);

	arma::vec acc_body_grav = args -> get_dyn_analyses() -> point_mass_acceleration(pos_body , args -> get_mass());
	arma::vec acc_body_frame = acc_body_grav - (2 * arma::cross(omega_TN, vel_body) + omega_TN * omega_TN.t() * pos_body - pos_body * omega_TN.t() * omega_TN);

	arma::vec dxdt = { X(3), X(4), X(5), acc_body_frame(0), acc_body_frame(1), acc_body_frame(2)};
	return dxdt;

}


arma::vec attitude_dxdt_wrapper(double t, arma::vec  X, Args * args) {

	arma::vec dxdt = RBK::dXattitudedt(t, X , args -> get_shape_model() -> get_inertia());

	return dxdt;

}


arma::vec validation_dxdt_wrapper(double t, arma::vec  X, Args * args) {

	arma::vec dxdt = {cos(t),sin(t)};

	return dxdt;

}

arma::vec event_function_mrp_omega(double t, arma::vec X, Args * args) {
	if (arma::norm(X.rows(0, 2)) > 1) {
		
		X.rows(0,2) = - X.rows(0, 2) / arma::dot(X . rows(0, 2), X . rows(0, 2));

		return X;
	}
	else {
		return X;
	}
}


arma::vec event_function_mrp(double t, arma::vec X, Args * args) {
	if (arma::norm(X.rows(0, 2)) > 1) {
		arma::vec mrp = - X.rows(0, 2) / arma::dot(X . rows(0, 2), X . rows(0, 2));
		return mrp;
	}
	else {
		return X;
	}
}

arma::vec event_function_collision(double t, arma::vec X, Args * args) {

	arma::vec mrp_TN = args -> get_interpolator() -> interpolate(t, true).rows(0, 2);

	args -> get_frame_graph() -> set_transform_mrp("N", "T", mrp_TN);
	arma::vec pos_inertial = X . rows(0, 2);
	arma::vec pos_body = args -> get_frame_graph() -> convert(pos_inertial, "N", "T");

	if (args -> get_shape_model() -> contains(pos_body.colptr(0))) {
		std::cout << " The spacecraft collided with the surface at time t = " << t << " s" << std::endl;
		args -> set_stopping_bool(true);
	}
	return X;
}


arma::vec event_function_collision_body_frame(double t, arma::vec X, Args * args) {


	arma::vec pos_body = X . rows(0, 2);

	if (args -> get_shape_model() -> contains(pos_body.colptr(0))) {
		std::cout << " The spacecraft collided with the surface at time t = " << t << " s" << std::endl;
		args -> set_stopping_bool(true);
	}
	return X;
}


double energy_attitude(double t, arma::vec X , Args * args) {

	arma::vec omega = X . rows(3, 5);

	return 0.5 * arma::dot(omega, args -> get_shape_model() -> get_inertia() * omega);

}

arma::vec joint_sb_spacecraft_body_frame_dyn(double t, arma::vec  X, Args * args){

	arma::vec dxdt(X.n_rows);

	arma::vec sigma = X.rows(0,3);
	arma::vec omega = X.rows(3,5);
	arma::vec pos = X.rows(6,8);
	arma::vec vel = X.rows(9,11);
	

	dxdt.rows(0,5) = attitude_dxdt_wrapper(t,X.rows(0,5),args);

	arma::vec omega_dot = dxdt.rows(3,5);

	arma::vec acc_sph = args -> get_dyn_analyses() -> spherical_harmo_acc(
		args -> get_degree(),
		args -> get_ref_radius(),
		args -> get_mu(),
		pos, 
		args -> get_Cnm(),
		args -> get_Snm());

	dxdt.rows(6,8) = X.rows(9,11);
	dxdt.rows(9,11) = (acc_sph - arma::cross(omega_dot,pos) - 2 * arma::cross(omega,vel)
		- arma::cross(omega,arma::cross(omega,pos)));


	return dxdt;

}








