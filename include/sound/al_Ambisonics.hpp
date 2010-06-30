#ifndef INCLUDE_AL_AMBISONICS_HPP
#define INCLUDE_AL_AMBISONICS_HPP

#include <stdio.h>
//#define MAX_ORDER 3

namespace al{

/// Ambisonic base class
class AmbiBase{
public:

	/// @param[in] dim		number of spatial dimensions (2 or 3)
	/// @param[in] order	highest spherical harmonic order
	AmbiBase(int dim, int order);

	virtual ~AmbiBase();

	int dim() const { return mDim; }
	int order() const { return mOrder; }
	const float * weights() const { return mWeights; }

	/// Returns total number of Ambisonic domain channels
	int channels() const { return mChannels; }
	
	void order(int order);		///< Set the order.
	
	virtual void onChannelsChange(){}	///< Called whenever the number of Ambisonic channels changes.
	
	static int channelsToUniformOrder(int channels);
	
	static void encodeWeightsFuMa(float * weights, int dim, int order, float azimuth, float elevation);
	static void encodeWeightsFuMa(float * ws, int dim, int order, float x, float y, float z);
	
	/// Brute force 3rd order.  Weights must be of size 16.
	static void encodeWeightsFuMa16(float * weights, float azimuth, float elevation);
	static void encodeWeightsFuMa16(float * ws, float x, float y, float z);
	
	static int orderToChannels(int dim, int order);
	static int orderToChannelsH(int orderH);
	static int orderToChannelsV(int orderV);
	
protected:
	int mDim;			// dimensions - 2d or 3d
	int mOrder;			// order - 0th, 1st, 2nd, or 3rd
	int mChannels;		// cached for efficiency
	float * mWeights;	// weights for each ambi channel
	
	static void resize(float * a, int n);
};


/// Higher Order Ambisonic Decoding class
class AmbiDecode : public AmbiBase{
public:
	AmbiDecode(int dim, int order, int numSpeakers, int flavor=1);
	virtual ~AmbiDecode();

	float decode(int speakerNum);	///< Decode speaker's sample from stored ambisonic frame.

	void decode(float ** dec, const float ** enc, int numDecFrames);

	void flavor(int type);
	void numSpeakers(int num);	///< Set number of speakers.  Positions are zeroed upon resize.
	void setSpeaker(int index, float azimuth, float elevation=0);
	void setSpeakerDegrees(int index, float azimuth, float elevation=0);
	void zero();				///< Zeroes out internal ambisonic frame.

	float * azimuths();				///< Returns pointer to speaker azimuths.
	float * elevations();			///< Returns pointer to speaker elevations.
	float * frame() const;			///< Returns pointer to ambisonic channel frame used by decode(int)
	int numSpeakers();		///< Returns number of speakers.
	int flavor();			///< Returns decode flavor.

	virtual void onChannelsChange();
	
	void print(FILE * fp = stdout, const char * append = "\n");
	
	float decodeWeight(int speaker, int channel){ 
		return mWeights[channel] * mDecodeMatrix[speaker * channels() + channel];
	}

protected:
	int mNumSpeakers;
	int mFlavor;				// decode flavor

	float * mDecodeMatrix;		// deccoding matrix for each ambi channel & speaker
								// cols are channels and rows are speakers								
	float mWOrder[5];			// weights for each order
	float * mPositions;			// speakers' azimuths + elevations
	float * mFrame;				// an ambisonic channel frame used for decode(int)

	void updateChanWeights();
	void resizeArrays(int numChannels, int numSpeakers);

	float decode(float * encFrame, int encNumChannels, int speakerNum);	// is this useful?
	
	static float flavorWeights[4][5][5];
};


/// Higher Order Ambisonic encoding class
class AmbiEncode : public AmbiBase{
public:

	/// Encode input sample and set decoder frame.
	void encode   (const AmbiDecode &dec, float input);
	
	/// Encode input sample and add to decoder frame.
	void encodeAdd(const AmbiDecode &dec, float input);

	template <class XYZ>
	void encode(float ** ambiChans, const XYZ * pos, const float * input, int numFrames){	
		for(int c=0; c<channels(); ++c){
			float * ambi = ambiChans[c];
			//T
			
			for(int i=0; i<numFrames; ++i){
				ambi[i] = weights()[c] * input[i];
			}
		}
	}
	
	/// Set spherical position of source to be encoded
	void position(float az, float el);
	
	/// Set Cartesian position of source to be encoded
	void position(float x, float y, float z);
};





// Implementation ______________________________________________________________


// AmbiBase
inline int AmbiBase::orderToChannels(int dim, int order){
	int chans = orderToChannelsH(order);
	return dim == 2 ? chans : chans + orderToChannelsV(order);
}

inline int AmbiBase::orderToChannelsH(int orderH){ return (orderH << 1) + 1; }
inline int AmbiBase::orderToChannelsV(int orderV){ return orderV * orderV; }


// AmbiEncode
inline void AmbiEncode::encode(const AmbiDecode &dec, float input){	
	for(int c=0; c<dec.channels(); ++c) dec.frame()[c] = weights()[c] * input;
}

inline void AmbiEncode::encodeAdd(const AmbiDecode &dec, float input){
	for(int c=0; c<dec.channels(); ++c) dec.frame()[c] += weights()[c] * input;
}

inline void AmbiEncode::position(float az, float el){
	AmbiBase::encodeWeightsFuMa(mWeights, mDim, mOrder, az, el);
}

inline void AmbiEncode::position(float x, float y, float z){
	AmbiBase::encodeWeightsFuMa(mWeights, mDim, mOrder, x,y,z);
}



// AmbiDecode

inline float AmbiDecode::decode(float * encFrame, int encNumChannels, int speakerNum){
	float smp = 0;
	float * dec = mDecodeMatrix + speakerNum * channels();
	float * wc = mWeights;
	for(int i=0; i<encNumChannels; ++i) smp += *dec++ * *wc++ * *encFrame++;
	return smp;
}

inline float AmbiDecode::decode(int speakerNum){
	return decode(mFrame, channels(), speakerNum);
}

inline float * AmbiDecode::azimuths(){ return mPositions; }
inline float * AmbiDecode::elevations(){ return mPositions + mNumSpeakers; }
inline float * AmbiDecode::frame() const { return mFrame; }
inline int AmbiDecode::flavor(){ return mFlavor; }
inline int AmbiDecode::numSpeakers(){ return mNumSpeakers; }

} // al::
#endif
