//
//  IOService.h
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2015 by Sidney Just
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
//  documentation files (the "Software"), to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
//  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#ifndef _IOSERVICE_H_
#define _IOSERVICE_H_

#include "../core/IOObject.h"
#include "../core/IORegistry.h"
#include "../core/IODictionary.h"
#include "../core/IOString.h"

namespace IO
{
	extern String *kServiceProviderMatchKey;
	extern String *kServiceClassMatchKey; // Ignored for now
	extern String *kServicePropertiesMatchKey;

	class Service : public RegistryEntry
	{
	public:
		static void InitialWakeUp(MetaClass *meta);
		static void RegisterService(MetaClass *meta, Dictionary *properties);

		virtual Service *InitWithProperties(Dictionary *properties); // Designated constructor

		virtual void Start();
		virtual void Stop();

		virtual bool MatchProperties(Dictionary *properties);
		virtual void PublishService(Service *service);

		void StartMatching();

		Dictionary *GetProperties() const;

	protected:
		void RegisterProvider();

		void AttachToParent(RegistryEntry *parent) override;

	private:
		void DoMatch();

		Dictionary *_properties;
		bool _started;

		IODeclareMeta(Service)
	};
}

#endif /* _IOSERVICE_H_ */
