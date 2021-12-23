/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Library General Public License for more details. To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/


#pragma once

#include <string>

/**
	\brief Indices for the types supported by the waveform generator parameters.
*/
enum wavegen_param_type_t {
	WGPARAM_UNSIGNED, /**< An unsigned integer value. */
	WGPARAM_FLOAT,    /**< A floating-point value. */
	WGPARAM_BOOLEAN,  /**< A boolean value. */
	WGPARAM_STRING,   /**< A string value. */
};

/**
	\brief A waveform generator parameter.
*/
class CWavegenParam {
protected:
	/** \brief Constructor of the waveform generator parameter.
		\param Type The type of the parameter. Provided in the constructor of derived classes.
		\param Name The name of the parameter. */
	CWavegenParam(wavegen_param_type_t Type, const char *Name);

public:
	/** \brief Virtual destructor of the waveform generator parameter. */
	virtual ~CWavegenParam();
	/** \brief Obtains the parameter's type.
		\return The type index of the parameter. */
	wavegen_param_type_t GetType() const;
	/** \brief Obtains the parameter's name.
		\return The name of the parameter. */
	const char* GetName() const;
	/** \brief Sets the value of the parameter.
		\details There is no corresponding getter; it is only required in the generator editor.
		\return True if the operation succeeded, false if it failed. */
	virtual bool SetValue(const void *Val) = 0;

protected:
	/** \brief The parameter type. */
	const wavegen_param_type_t m_iParamType;
	/** \brief A description of the parameter. */
	const char* m_pParamName;
};

/**
	\brief Unsigned integer specialization of the waveform generator parameter.
*/
class CWavegenParamUnsigned : public CWavegenParam {
public:
	CWavegenParamUnsigned(const char *Name);
	/** \brief Obtains the parameter's value.
		\return The parameter value as an unsigned integer. */
	unsigned int GetValue() const;
	virtual bool SetValue(const void *Val);

private:
	unsigned int m_iData;
};

/**
	\brief Floating-point value specialization of the waveform generator parameter.
*/
class CWavegenParamFloat : public CWavegenParam {
public:
	CWavegenParamFloat(const char *Name);
	/** \brief Obtains the parameter's value.
		\return The parameter value as a floating-point value. */
	float GetValue() const;
	virtual bool SetValue(const void *Val);

private:
	float m_fData;
};

/**
	\brief Boolean specialization of the waveform generator parameter.
*/
class CWavegenParamBoolean : public CWavegenParam {
public:
	CWavegenParamBoolean(const char *Name);
	/** \brief Obtains the parameter's value.
		\return The parameter value as a boolean. */
	bool GetValue() const;
	virtual bool SetValue(const void *Val);

private:
	bool m_bData;
};

/**
	\brief String specialization of the waveform generator parameter.
*/
class CWavegenParamString : public CWavegenParam {
public:
	CWavegenParamString(const char *Name);
	/** \brief Obtains the parameter's value.
		\return The parameter value as a string. */
	const char *GetValue() const;
	virtual bool SetValue(const void *Val);

private:
	static const size_t MAX_LENGTH;
	std::string m_pData;
};

/**
	\brief A waveform generator for instruments supporting wave tables.
*/
class CWaveformGenerator {
public:
	/** \brief Virtual destructor of the waveform generator. */
	virtual ~CWaveformGenerator() { }
	/** \brief Generates floating-point waveforms.
		\details Successive floating-point values are written to an array for further quantization.
		The waveforms should be expected to have a range between -1 and 1.
		\param Dest Pointer to a floating-point array.
		\param Size The number of samples per waveform.
		\param Index The waveform number to generate, or -1 to generate all waveforms consecutively.
		\return True if the operation succeeded, false if it failed. */
	virtual bool CreateWaves(float *const Dest, unsigned int Size, unsigned int Index) = 0;
	/** \brief Obtains a parameter object.
		\details Out-of-bound indices \b must return nullptr to signal the end of the waveform
		generator's parameter list.
		\param Index A unique numerical index.
		\return Pointer to a parameter of the waveform generator. */
	virtual CWavegenParam *GetParameter(unsigned int Index) const = 0;
	/** \brief Obtains the expected number of waves generated.
		\details This may depend on the waveform generator's own parameter list.
		\return The wave count of the waveform generator. */
	virtual unsigned int GetCount() const = 0;
	/** \brief Returns a status describing the last call to CreateWaves.
		\details This function should return an empty string if no message is to be displayed on.
		success. If waveform generation fails, the generator dialog shows this message and prevents
		the user from exiting the dialog.
		\return The status string. */
	virtual const char *GetStatus() const = 0;
	/** \brief Obtains the generator's name.
		\return The name of the waveform generator. */
	virtual const char *GetGeneratorName() const = 0;
};