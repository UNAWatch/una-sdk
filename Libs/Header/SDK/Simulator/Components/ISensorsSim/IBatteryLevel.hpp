/**
 ******************************************************************************
 * @file    IBattLevel.hpp
 * @date    11-March-2026
 * @author  Vlad Andriyash
 * @brief   IBattLevel interface
 * 
 ******************************************************************************
 *
 ******************************************************************************
 */

#ifndef __I_BATTERY_LEVEL_INTERFACE_HPP
#define __I_BATTERY_LEVEL_INTERFACE_HPP

namespace Interface
{
    class IBattLevel
    {
    public:
		/**
         * @brief   Destructor.
         */
		virtual ~IBattLevel() = default;
	
	    /**
		 * @brief   Set Param.
		 * @param   startValue: percent baatery which start simulation.
		 *          stepValue: value which deduct from startValue
		 */
        virtual void setParam(float startValue, float stepValue) = 0;
		
		/**
		 * @brief   Get battery value.
		 * @return  battery value in percent.
		 */
        virtual float getBattLevel() = 0;
    }; /* class IBattLevel */
} /* namespace Interface */

#endif /* __I_BATTERY_LEVEL_INTERFACE_HPP */
