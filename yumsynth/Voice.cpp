/*
 * Voice.cpp
 *
 * Represents a single voice with 4 operators. See Voice.h for details.
 *
 * @author Andrew Ford
 */

#include "Voice.h"
#include "Operator.h"

#include <math.h>

Voice::Voice( float* freq, int numOps )
	: frequencyTable( freq ), numOperators( numOps )
{
	note = -1;
	// assume dummy samplerate at first, setSamplerate() will get called by
	// host
	samplerate = 48000;

	for( int i = 0; i < numOperators; i++ )
	{
		Operator* o = new Operator();
		o->setSamplerate( samplerate );
		operators.push_back( o );
	}

	operatorArrangement = 0;
	numOperatorArrangements = 10;

	operatorArrangementDescriptions.push_back( std::string( "1>2>3>4" ) );
	operatorArrangementDescriptions.push_back( std::string( "1+2>3>4" ) );
	operatorArrangementDescriptions.push_back( std::string( "1+(2>3)>4" ) );
	operatorArrangementDescriptions.push_back( std::string( "(1>2)+3>4" ) );
	operatorArrangementDescriptions.push_back( std::string( "1>2,3>4" ) );
	operatorArrangementDescriptions.push_back( std::string( "1>2,1>3,1>4" ) );
	operatorArrangementDescriptions.push_back( std::string( "1>2,3,4" ) );
	operatorArrangementDescriptions.push_back( std::string( "1,2,3,4" ) );
	operatorArrangementDescriptions.push_back( std::string( "1>2" ) );
	operatorArrangementDescriptions.push_back( std::string( "1" ) );
}

Voice::~Voice()
{
	for( int i = 0; i < numOperators; i++ )
	{
		delete operators[ i ];
	}
}

float Voice::evaluate()
{
	float out = 0.0f;

	for( unsigned int i = 0; i < slots.size(); i++ )
	{
		out += slots[ i ]->evaluate();
	}
	// divide by number of slots to mix properly
	out = slots.size() > 1 ? out / (float)slots.size() : out;

	// need to do postevaluate to clear caches, increment envelopes etc.
	for( unsigned int i = 0; i < operators.size(); i++ )
	{
		operators[ i ]->postEvaluate();
	}

	return out;
}

void Voice::noteOn( int n )
{
	// if invalid note val, return
	if ( n < 0 || n > 127 )
	{
		return;
	}
	note = n;
	
	float frequency = frequencyTable[ note ];
	for( unsigned int i = 0; i < operators.size(); i++ )
	{
		operators[ i ]->noteOn( frequency );
	}
}

void Voice::noteOff()
{
	note = -1;
	for( unsigned int i = 0; i < operators.size(); i++ )
	{
		operators[ i ]->noteOff();
	}
}

void Voice::setSamplerate( int sr )
{
	samplerate = sr;
	for( unsigned int i = 0; i < operators.size(); i++ )
	{
		operators[ i ]->setSamplerate( sr );
	}
}

int Voice::getNote()
{
	return note;
}

void Voice::setOperatorArrangement( int type )
{
	if ( type < 0 || type > numOperatorArrangements ||
		type == operatorArrangement )
	{
		return;
	}
	operatorArrangement = type;

	slots.clear();
	for( unsigned int i = 0; i < operators.size(); i++ )
	{
		operators[ i ]->resetInputOperators();
	}
	
	// set up operator arrangment by both setting slots (final outputs) and
	// linking operators together. kind of arbitrary, not necessarily
	// generalizable
	switch( type )
	{
	case 0:
		slots.push_back( operators[ operators.size() - 1 ] );
		for( unsigned int i = operators.size() - 1; i > 0; i-- )
		{
			operators[ i ]->addInputOperator( operators[ i - 1 ] );
		}
		break;
	case 1:
		slots.push_back( operators[ 3 ] );
		operators[ 3 ]->addInputOperator( operators[ 2 ] );
		operators[ 2 ]->addInputOperator( operators[ 0 ] );
		operators[ 2 ]->addInputOperator( operators[ 1 ] );
		break;
	case 2:
		slots.push_back( operators[ 3 ] );
		operators[ 3 ]->addInputOperator( operators[ 0 ] );
		operators[ 3 ]->addInputOperator( operators[ 2 ] );
		operators[ 2 ]->addInputOperator( operators[ 1 ] );
		break;
	case 3:
		slots.push_back( operators[ 3 ] );
		operators[ 3 ]->addInputOperator( operators[ 2 ] );
		operators[ 3 ]->addInputOperator( operators[ 1 ] );
		operators[ 2 ]->addInputOperator( operators[ 0 ] );
		break;
	case 4:
		// note, have to cast to (signed) int since the loop will check for a
		// negative i
		for( int i = (int)operators.size() - 1; i > 0; i -= 2 )
		{
			slots.push_back( operators[ i ] );
			operators[ i ]->addInputOperator( operators[ i - 1 ] );
		}
		break;
	case 5:
		for( unsigned int i = 1; i < operators.size(); i++ )
		{
			slots.push_back( operators[ i ] );
			operators[ i ]->addInputOperator( operators[ 0 ] );
		}
		break;
	case 6:
		for( unsigned int i = 1; i < operators.size(); i++ )
		{
			slots.push_back( operators[ i ] );
		}
		operators[ 1 ]->addInputOperator( operators[ 0 ] );
		break;
	case 7:
		for( unsigned int i = 0; i < operators.size(); i++ )
		{
			slots.push_back( operators[ i ] );
		}
		break;
	case 8:
		slots.push_back( operators[ 1 ] );
		operators[ 1 ]->addInputOperator( operators[ 0 ] );
		break;
	case 9:
		slots.push_back( operators[ 0 ] );
		break;
	}
}

int Voice::getOperatorArrangement()
{
	return operatorArrangement;
}

int Voice::getNumOperatorArrangements()
{
	return numOperatorArrangements;
}

std::string Voice::getOperatorArrangementDescription( int type )
{
	if ( type < 0 || type >= numOperatorArrangements )
	{
		return "";
	}
	return operatorArrangementDescriptions[ type ];
}

void Voice::setOperatorParam( int op, int param, float value )
{
	if ( op < 0 || op >= numOperators )
	{
		return;
	}
	operators[ op ]->setParam( param, value );
}

float Voice::getOperatorParam( int op, int param )
{
	if ( op < 0 || op >= numOperators )
	{
		return -9999.9f;
	}
	return operators[ op ]->getParam( param );
}

bool Voice::isPlaying()
{
	bool playing = false;
	// aggregate values from output operators - if one is playing, voice is
	// playing
	for( unsigned int i = 0; i < slots.size(); i++ )
	{
		playing = playing || slots[ i ]->isPlaying();
	}
	return playing;
}
