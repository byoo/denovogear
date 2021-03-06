/*
 * Copyright (c) 2014 Reed A. Cartwright
 * Authors:  Reed A. Cartwright <reed@cartwrig.ht>
 *
 * This file is part of DeNovoGear.
 *
 * DeNovoGear is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
#ifndef DNG_PEDIGREE_H
#define DNG_PEDIGREE_H

#include <functional>
#include <cmath>
#include <array>

#include <dng/matrix.h>
#include <dng/io/ped.h>
#include <dng/newick.h>
#include <dng/read_group.h>

namespace dng {

class Pedigree {
public:
	struct params_t {
		//params_t() : theta{0.001}, mu{1e-9}, ref_weight{0.0}, nuc_freq{0.25,0.25,0.25,0.25}
		//	{ }
		double theta;
		double mu;
		double mu_somatic;
		double mu_pcr;
		double ref_weight;
		std::array<double,4> nuc_freq;
	};

	bool Initialize(params_t p);

	bool Construct(const io::Pedigree& pedigree, const dng::ReadGroups& rgs);

	double LogPeelAll(const TransitionVector &mat, int ref) {
		// Copy genotype Priors
		// TODO: use a different prior based on reference
		std::fill(upper_.begin(), upper_.begin()+num_members_, genotype_prior_[ref]);
		
		// Peel pedigree one family at a time
		for(std::size_t i = 0; i < peeling_op_.size(); ++i)
			//peeling_op_[i](this, family_members_[i]);
			(this->*peeling_op_[i])(family_members_[i], mat);
			
		// Sum over roots
		double ret = 0.0;
		for(auto r : roots_) {
			ret += log((lower_[r]*upper_[r]).sum());
		}
		return ret;
	}

	double CalculateLogLikelihood(int ref) {
		return LogPeelAll(full_transition_matrices_,ref);
	}
	
	double CalculateMutProbability(int ref) {
		double bottom = LogPeelAll(full_transition_matrices_,ref);
		double top = LogPeelAll(nomut_transition_matrices_,ref);
#ifdef NDEBUG
		//NOTE: fast math may disable the trick below to avoid negative zero.
		//      Use abs to work around it.
		return std::abs(std::expm1(top-bottom));
#else
		// Avoid -0, but if something goes wrong still return negative probabilities
		return 0.0-std::expm1(top-bottom);
#endif
	}	

protected:
	typedef std::vector<size_t> family_members_t;
	typedef std::vector<size_t> peeling_matrices_t;

	std::vector<family_members_t> family_members_;
	std::size_t num_members_, num_libraries_, num_nodes_;
	std::vector<std::size_t> roots_;
	
	TransitionMatrix mitosis_, pcr_;
	TransitionMatrix meiosis_, meiosis_nomut_;
	TransitionVector full_transition_matrices_;
	TransitionVector nomut_transition_matrices_;

	GenotypeArray genotype_prior_[5]; // Holds P(G | theta)
	PairedGenotypeArray buffer_;

	IndividualBuffer upper_; // Holds P(Data & G=g)
	IndividualBuffer lower_; // Holds P(Data | G=g)

public:
	decltype(lower_)::reference library_lower(std::size_t k) {
		return lower_[num_members_+k];
	}

	std::size_t size() const { return num_nodes_; }

protected:
	void PeelUp(const family_members_t &family_members, const TransitionVector &mat) {
		lower_[family_members[0]] = (mat[family_members[1]] * lower_[family_members[1]].matrix()).array();
	}

	void PeelUp2(const family_members_t &family_members, const TransitionVector &mat) {
		lower_[family_members[0]] *= (mat[family_members[1]] * lower_[family_members[1]].matrix()).array();
	}

	void PeelDown(const family_members_t &family_members, const TransitionVector &mat) {
		upper_[family_members[1]].matrix() = mat[family_members[1]].transpose() *
			(upper_[family_members[0]]*lower_[family_members[0]]).matrix();
	}

	void PeelToFather(const family_members_t &family_members, const TransitionVector &mat) {
		// Sum over children
		buffer_ = (mat[family_members[2]] * lower_[family_members[2]].matrix()).array();
		for(std::size_t i = 3; i < family_members.size(); i++) {
			buffer_ *= (mat[family_members[i]] * lower_[family_members[i]].matrix()).array();
		}
		// Include Mom
		buffer_.resize(10,10);
		lower_[family_members[0]] = (buffer_.matrix() *
			(upper_[family_members[1]]*lower_[family_members[1]]).matrix()).array();		
		buffer_.resize(100,1);

		//Eigen::Map<Matrix10d, Eigen::Aligned> mat(buffer_.data());
		//lower_[family_members[0]] = (mat *
		//	(upper_[family_members[1]]*lower_[family_members[1]]).matrix()).array();		
	}

	void PeelToFather2(const family_members_t &family_members, const TransitionVector &mat) {
		// Sum over children
		buffer_ = (mat[family_members[2]] * lower_[family_members[2]].matrix()).array();
		for(std::size_t i = 3; i < family_members.size(); i++) {
			buffer_ *= (mat[family_members[i]] * lower_[family_members[i]].matrix()).array();
		}
		// Include Mom
		buffer_.resize(10,10);
		lower_[family_members[0]] *= (buffer_.matrix() *
			(upper_[family_members[1]]*lower_[family_members[1]]).matrix()).array();		
		buffer_.resize(100,1);
	}

	void PeelToMother(const family_members_t &family_members, const TransitionVector &mat) {
		// Sum over children
		buffer_ = (mat[family_members[2]] * lower_[family_members[2]].matrix()).array();
		for(std::size_t i = 3; i < family_members.size(); i++) {
			buffer_ *= (mat[family_members[i]] * lower_[family_members[i]].matrix()).array();
		}
		// Include Dad
		buffer_.resize(10,10);
		lower_[family_members[1]] = (buffer_.matrix().transpose() *
			(upper_[family_members[0]]*lower_[family_members[0]]).matrix()).array();		
		buffer_.resize(100,1);

		//Eigen::Map<RowMatrix10d, Eigen::Aligned> mat(buffer_.data());
		//lower_[family_members[1]] = (mat *
		//	(upper_[family_members[0]]*lower_[family_members[0]]).matrix()).array();
	}

	void PeelToMother2(const family_members_t &family_members, const TransitionVector &mat) {
		// Sum over children
		buffer_ = (mat[family_members[2]] * lower_[family_members[2]].matrix()).array();
		for(std::size_t i = 3; i < family_members.size(); i++) {
			buffer_ *= (mat[family_members[i]] * lower_[family_members[i]].matrix()).array();
		}
		// Include Dad
		buffer_.resize(10,10);
		lower_[family_members[1]] *= (buffer_.matrix().transpose() *
			(upper_[family_members[0]]*lower_[family_members[0]]).matrix()).array();		
		buffer_.resize(100,1);
	}

	void PeelToChild(const family_members_t &family_members, const TransitionVector &mat) {
		assert(family_members.size()==3);
		// Parents
		buffer_ = kroneckerProduct(
			(lower_[family_members[0]] * upper_[family_members[0]]).matrix(),
			(lower_[family_members[1]] * upper_[family_members[1]]).matrix()
		).array();
		
		upper_[family_members[2]].matrix() = mat[family_members[2]].transpose() * buffer_.matrix();
	}
	void PeelToChild2(const family_members_t &family_members, const TransitionVector &mat) {
		assert(family_members.size()>=4);
		buffer_ = (mat[family_members[3]] * lower_[family_members[3]].matrix()).array();
		// Sum over children
		for(std::size_t i = 4; i < family_members.size(); i++) {
			buffer_ *= (mat[family_members[i]] * lower_[family_members[i]].matrix()).array();
		}
		// Parents
		buffer_ *= kroneckerProduct(
			(lower_[family_members[0]] * upper_[family_members[0]]).matrix(),
			(lower_[family_members[1]] * upper_[family_members[1]]).matrix()
		).array();
		
		upper_[family_members[2]].matrix() = mat[family_members[2]].transpose() * buffer_.matrix();
	}

	//typedef decltype(std::mem_fn(&Pedigree::PeelUp)) PeelOp;
	typedef decltype(&Pedigree::PeelUp) PeelOp;

	typedef Pedigree Op;

	std::vector<PeelOp> peeling_op_;	
};

}; // namespace dng


#endif // DNG_PEDIGREE_H
