/** @file inputdesc.h
	@brief Substance input description structure
	@author Christophe Soum - Allegorithmic (christophe.soum@allegorithmic.com)
	@note This file is part of the Substance engine headers
	@date 20080107
	@copyright Allegorithmic. All rights reserved.

	Defines the SubstanceInputDesc structure. Filled via the
	substanceHandleGetInputDesc function declared in handle.h.
*/

#ifndef _SUBSTANCE_INPUTDESC_H
#define _SUBSTANCE_INPUTDESC_H


/** @brief Substance input type enumeration

	Mutually exclusive values. 4 bits format */
typedef enum
{
	Substance_IType_Float    = 0x0, /**< Float (scalar) type */
	Substance_IType_Float2   = 0x1, /**< 2D Float (vector) type */
	Substance_IType_Float3   = 0x2, /**< 3D Float (vector) type */
	Substance_IType_Float4   = 0x3, /**< 4D Float (vector) type (e.g. color) */
	
	Substance_IType_Integer  = 0x4, /**< Integer type (int 32bits, enum or bool) */
	Substance_IType_Integer2 = 0x8, /**< 2D Integer (vector) type */
	Substance_IType_Integer3 = 0x9, /**< 3D Integer (vector) type */
	Substance_IType_Integer4 = 0xA, /**< 4D Integer (vector) type */	
	
	Substance_IType_Image    = 0x5, /**< Compositing entry: bitmap/texture data */
	
	Substance_IType_Mesh     = 0x6, /**< Mesh type: Not yet used */
	Substance_IType_String   = 0x7  /**< NULL terminated char string: Not yet used */
	
} SubstanceInputType;


/** @brief Substance input description structure definition

	Filled using the substanceHandleGetInputDesc function declared in handle.h. */
typedef struct SubstanceInputDesc_
{
	/** @brief Input unique identifier */
	unsigned int inputId;
	
	/** @brief Input type */
	SubstanceInputType inputType;
	
	/** @brief Current value of the Input. Depending on the type. */
	union
	{
		/** @brief Pointer on the current value of FloatX type inputs 
			@note Valid only if inputType is Float Float2 Float3 or Float4 */
		const float *typeFloatX;   
		
		/** @brief Pointer on the current value of Integer type input 
			@note Valid only if inputType is Integer or Integer2,3,4 */
		const int *typeIntegerX;
		
		/** @brief Pointer on the current ASCII string set to String type input
			@note Valid only if inputType is Substance_IType_String */
		const char **typeString;   
		
	} value;

} SubstanceInputDesc;

#endif /* ifndef _SUBSTANCE_INPUTDESC_H */
