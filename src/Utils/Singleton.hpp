/*
 * Singleton.hpp
 *
 *  Created on: 27.08.2017
 *      Author: christoph
 */

#ifndef SRC_UTILS_SINGLETON_HPP_
#define SRC_UTILS_SINGLETON_HPP_

#include <memory>

namespace sgl {

// Singleton instance of classes T derived from Singleton<T> can be accessed using T::get().

template <class T>
class Singleton
{
public:
	virtual ~Singleton () { }
	inline static void deleteSingleton() { singleton = std::unique_ptr<T>(); }

	// Creates static instance if necessary and returns the pointer to it
	inline static T *get()
	{
		if (!singleton.get())
			singleton = std::unique_ptr<T>(new T);
		return singleton.get();
	}

protected:
	static std::unique_ptr<T> singleton;
};

template <class T>
std::unique_ptr<T> Singleton<T>::singleton;

}

#endif /* SRC_UTILS_SINGLETON_HPP_ */
