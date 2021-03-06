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
#ifndef CXX_HTS_BAM_H
#define CXX_HTS_BAM_H

#include "hts.h"

#include <htslib/sam.h>

namespace hts { namespace bam {

typedef bam1_t BareAlignment;

class File;

typedef std::pair<const uint32_t*,const uint32_t*> cigar_t;
typedef std::pair<const uint8_t*,const uint8_t*> data_t;

class Alignment : protected BareAlignment {
public:
	Alignment() {
		data = nullptr;
		m_data = 0;
		l_data = 0;		
	}

	Alignment(const Alignment& other) {
		data = nullptr;
		m_data = 0;
		bam_copy1(base(),other.base());
	}

	Alignment(Alignment&& other) : BareAlignment(*other.base()) {
		other.data = nullptr;
		other.l_data = other.m_data = 0;
	}

	~Alignment() {
		free(data);
	}

	Alignment& operator=(const Alignment& other) {
		if(this != &other)
			bam_copy1(base(),other.base());
		return *this;
	}

	Alignment& operator=(Alignment&& other) {
		if(this == &other)
			return *this;
		free(data);
		*base() = *other.base();
		other.data = nullptr;
		other.l_data = other.m_data = 0;
		return *this;
	}

	inline uint32_t target_id() const { return core.tid; }
	inline uint32_t position() const { return core.pos; }
	inline uint32_t map_qual() const { return core.qual; }
	inline uint32_t mate_target_id() const { return core.mtid; }
	inline uint32_t mate_position() const { return core.mpos; }
	inline uint16_t flags() const { return core.flag; }

	inline bool is_any(uint16_t f) const { return ((flags() & f) != 0); }
	inline bool are_only(uint16_t f) const { return (flags() == f); }
	inline bool are_all(uint16_t f) const { return ((flags() & f) == f); }

	inline bool is_reversed() const { return is_any(BAM_FREVERSE); }
	inline bool mate_is_reversed() const { return is_any(BAM_FMREVERSE); }

	inline const char* qname() const {
		return bam_get_qname(this);
	}
	inline cigar_t cigar() const {
		const uint32_t* b = bam_get_cigar(this);
		return std::make_pair(b,b+core.n_cigar);
	}
	//TODO: Make seq-specific iterator
	inline data_t seq() const {
		const uint8_t* b = bam_get_seq(this);
		return std::make_pair(b,b+(core.l_qseq+1)/2);
	}
	inline data_t seq_qual() const {
		const uint8_t* b = bam_get_qual(this);
		return std::make_pair(b,b+core.l_qseq);
	}
	inline data_t aux() const {
		const uint8_t* b = bam_get_aux(this);
		return std::make_pair(b,b+bam_get_l_aux(this));
	}

	inline uint8_t seq_at(int32_t x) const {
		assert(0 <= x && x < core.l_qseq);
		return bam_seqi(bam_get_seq(this),x);
	}

	inline uint8_t* aux_get(const char tag[3]) {
		return bam_aux_get(base(), tag);
	}

protected:
	BareAlignment* base() {return static_cast<BareAlignment*>(this);}
	const BareAlignment* base() const {return static_cast<const BareAlignment*>(this);}

	friend class File;
};

class File : public hts::File {
public:
	File(const char *file, const char *mode, const char *region=nullptr, const char *fasta=nullptr,
		int min_mapQ = 0, int min_len = 0) : hts::File(file,mode),
			hdr_(nullptr), iter_(nullptr), min_mapQ_(min_mapQ), min_len_(min_len) {
		// TODO: handle different modes here???
		// TODO: remove throws or move some of them to base class???
	    if(!is_open())
	    	throw std::runtime_error("unable to open file '" + std::string(file) + "'.");
	    SetFaiFileName(fasta);
	    hdr_ = sam_hdr_read(handle());
	    if(hdr_ == nullptr)
	    	throw std::runtime_error("unable to read header in file '" + std::string(file) + "'.");
	    if(region != nullptr && region[0] != '\0') {
	    	hts_idx_t *idx = sam_index_load(handle(), file);
	    	if(idx == nullptr)
	    		throw std::runtime_error("unable to load index for '" + std::string(file) + "'.");
	    	iter_ = sam_itr_querys(idx, hdr_, region);
	    	hts_idx_destroy(idx);
	    	if(iter_ == nullptr) {
	    		throw std::runtime_error("unable to parse region '" + std::string(region) + "'.");
	    	}
	    }
	}
	File(File&& other) : hts::File(std::move(other)), hdr_(other.hdr_),
		iter_(other.iter_), min_mapQ_(other.min_mapQ_), min_len_(other.min_len_)
	{
		other.hdr_ = nullptr;
		other.iter_ = nullptr;

	}
	File(const File&) = delete;
	
	File& operator=(File&& other) {
		if(this == &other) 
			return *this;

		hts::File::operator=(std::move(other));

		if(iter_ != nullptr)
			hts_itr_destroy(iter_);
		iter_ = other.iter_;
		other.iter_ = nullptr;

		if(hdr_ != nullptr)
			bam_hdr_destroy(hdr_);
		hdr_ = other.hdr_;
		other.hdr_ = nullptr;

		min_mapQ_ = other.min_mapQ_;
		min_len_  = other.min_len_;

		return *this;
	}
	File& operator=(const File&) = delete;
	
	int Read(Alignment& a) {
		int ret;
		for(;;) {
		    ret = (iter_) ? sam_itr_next(handle(), iter_, a.base())
		                  : sam_read1(handle(), hdr_, a.base());
		    if (ret < 0)
		    	break;
		    if (a.is_any(BAM_FUNMAP | BAM_FSECONDARY | BAM_FQCFAIL | BAM_FDUP))
		    	continue;
		    if (a.map_qual() < min_mapQ_)
		    	continue;
		    // TODO: Move this to the outer loop?
		    if (min_len_ > 0) {
		    	auto p = a.cigar();
		    	if(bam_cigar2qlen(p.second-p.first, p.first) < min_len_)
		    		continue;
		    }
		    break;
		}
		return ret;
	}
	int operator()(Alignment& a) { return Read(a); }

	int Write(const bam1_t& b) {
		return sam_write1(handle(), hdr_, &b);
	}

	// cleanup as needed
	virtual ~File() {
		if(iter_ != nullptr)
			hts_itr_destroy(iter_);
		if(hdr_ != nullptr)
			bam_hdr_destroy(hdr_);
	}
	
	const bam_hdr_t * header() const { return hdr_; }
	const hts_itr_t * iter() const { return iter_; }
	
protected:
	bam_hdr_t *hdr_;  // the file header
	hts_itr_t *iter_; // NULL if a region not specified
	int min_mapQ_, min_len_; // mapQ filter; length filter
};

} // namespace bam

} // namespace hts

#endif