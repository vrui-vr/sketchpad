/***********************************************************************
SketchObjectList - Class for lists of sketching objects in a simple
sketching application.
Copyright (c) 2016-2019 Oliver Kreylos
***********************************************************************/

#ifndef SKETCHOBJECTLIST_INCLUDED
#define SKETCHOBJECTLIST_INCLUDED

#include "SketchObject.h"

class SketchObjectList
	{
	/* Embedded classes: */
	public:
	class const_iterator; // Forward declaration
	
	class iterator // Iterator through lists
		{
		friend class SketchObjectList;
		friend class const_iterator;
		
		/* Elements: */
		private:
		SketchObject* obj; // Pointer to iterated sketch object
		
		/* Constructors and destructors: */
		public:
		iterator(void) // Creates an invalid iterator
			:obj(0)
			{
			}
		iterator(SketchObject* sObj) // Creates an iterator to the given sketch object
			:obj(sObj)
			{
			}
		
		/* Methods: */
		bool operator==(const iterator& other) const
			{
			return obj==other.obj;
			}
		bool operator!=(const iterator& other) const
			{
			return obj!=other.obj;
			}
		SketchObject& operator*(void) const
			{
			return *obj;
			}
		SketchObject* operator->(void) const
			{
			return obj;
			}
		iterator& operator++(void)
			{
			if(obj!=0)
				obj=obj->succ;
			return *this;
			}
		};
	
	class const_iterator // Iterator through constant lists
		{
		friend class SketchObjectList;
		
		/* Elements: */
		private:
		const SketchObject* obj; // Pointer to iterated sketch object
		
		/* Constructors and destructors: */
		public:
		const_iterator(void) // Creates an invalid iterator
			:obj(0)
			{
			}
		const_iterator(const SketchObject* sObj) // Creates an iterator to the given sketch object
			:obj(sObj)
			{
			}
		const_iterator(const const_iterator& source) // Converts regular iterator to constant iterator
			:obj(source.obj)
			{
			}
		
		/* Methods: */
		public:
		bool operator==(const const_iterator& other) const
			{
			return obj==other.obj;
			}
		bool operator!=(const const_iterator& other) const
			{
			return obj!=other.obj;
			}
		const SketchObject& operator*(void) const
			{
			return *obj;
			}
		const SketchObject* operator->(void) const
			{
			return obj;
			}
		const_iterator& operator++(void)
			{
			if(obj!=0)
				obj=obj->succ;
			return *this;
			}
		};
	
	class reverse_iterator // Iterator through lists from end to beginning
		{
		friend class SketchObjectList;
		
		/* Elements: */
		private:
		SketchObject* obj; // Pointer to iterated sketch object
		
		/* Constructors and destructors: */
		public:
		reverse_iterator(void) // Creates an invalid reverse_iterator
			:obj(0)
			{
			}
		reverse_iterator(SketchObject* sObj) // Creates an reverse_iterator to the given sketch object
			:obj(sObj)
			{
			}
		
		/* Methods: */
		bool operator==(const reverse_iterator& other) const
			{
			return obj==other.obj;
			}
		bool operator!=(const reverse_iterator& other) const
			{
			return obj!=other.obj;
			}
		SketchObject& operator*(void) const
			{
			return *obj;
			}
		SketchObject* operator->(void) const
			{
			return obj;
			}
		reverse_iterator& operator++(void)
			{
			if(obj!=0)
				obj=obj->pred;
			return *this;
			}
		};
	
	/* Elements: */
	private:
	SketchObject* head; // Pointer to first sketch object in list or 0 for empty list
	SketchObject* tail; // Pointer to last sketch object in list or 0 for empty list
	
	/* Private methods: */
	void unlink(SketchObject* obj) // Removes the given sketch object from the list
		{
		/* Unlink the sketch object from the list: */
		if(obj->pred!=0)
			obj->pred->succ=obj->succ;
		else
			head=obj->succ;
		if(obj->succ!=0)
			obj->succ->pred=obj->pred;
		else
			tail=obj->pred;
		}
	
	/* Constructors and destructors: */
	public:
	SketchObjectList(void) // Creates an empty list
		:head(0),tail(0)
		{
		}
	private:
	SketchObjectList(const SketchObjectList& source); // Prohibit copy constructor
	SketchObjectList& operator=(const SketchObjectList& source); // Prohibit assignment operator
	public:
	~SketchObjectList(void); // Destroys a list and all contained sketch objects
	
	/* Methods: */
	bool empty(void) const // Returns true if the list is empty
		{
		return head==0;
		}
	size_t size(void) const; // Returns number of elements in list; slow
	const_iterator begin(void) const // Returns a constant iterator to the first list element
		{
		return const_iterator(head);
		}
	iterator begin(void) // Returns a regular iterator to the first list element
		{
		return iterator(head);
		}
	const_iterator end(void) const // Returns a constant iterator to behind the last element
		{
		return const_iterator(0);
		}
	iterator end(void) // Returns a regular iterator to behind the last element
		{
		return iterator(0);
		}
	reverse_iterator rbegin(void) // Returns a reverse iterator to the last list element
		{
		return reverse_iterator(tail);
		}
	reverse_iterator rend(void) const // Returns a reverse iterator to before the first element
		{
		return reverse_iterator(0);
		}
	void clear(void); // Clears the list and destroys all sketch objects
	SketchObjectList& push_back(SketchObject* newObject) // Appends the given sketch object to the end of the list
		{
		if(tail!=0)
			tail->succ=newObject;
		else
			head=newObject;
		newObject->pred=tail;
		tail=newObject;
		return *this;
		}
	SketchObjectList& insert(const iterator& insertIt,SketchObject* newObject) // Inserts the given sketch object before the given iterator
		{
		if(insertIt.obj!=0)
			{
			newObject->pred=insertIt.obj->pred;
			newObject->succ=insertIt.obj;
			}
		else
			newObject->pred=tail;
		if(newObject->pred!=0)
			newObject->pred->succ=newObject;
		else
			head=newObject;
		if(newObject->succ!=0)
			newObject->succ->pred=newObject;
		else
			tail=newObject;
		return *this;
		}
	SketchObjectList& erase(const iterator& eraseIt) // Removes the iterated sketch object from the list and destroys it
		{
		/* Unlink the sketch object from the list: */
		unlink(eraseIt.obj);
		
		/* Delete the sketch object: */
		delete eraseIt.obj;
		
		return *this;
		}
	SketchObjectList& erase(const reverse_iterator& eraseIt) // Removes the iterated sketch object from the list and destroys it
		{
		/* Unlink the sketch object from the list: */
		unlink(eraseIt.obj);
		
		/* Delete the sketch object: */
		delete eraseIt.obj;
		
		return *this;
		}
	SketchObject* unlink(const iterator& unlinkIt) // Removes the iterated object from the list and returns it as a singleton
		{
		/* Unlink the sketch object from the list: */
		unlink(unlinkIt.obj);
		
		/* Convert the sketch object to a singleton and return it: */
		unlinkIt.obj->pred=0;
		unlinkIt.obj->succ=0;
		
		return unlinkIt.obj;
		}
	SketchObject* unlink(const reverse_iterator& unlinkIt) // Removes the iterated object from the list and returns it as a singleton
		{
		/* Unlink the sketch object from the list: */
		unlink(unlinkIt.obj);
		
		/* Convert the sketch object to a singleton and return it: */
		unlinkIt.obj->pred=0;
		unlinkIt.obj->succ=0;
		
		return unlinkIt.obj;
		}
	void transfer(SketchObjectList& receiver); // Transfers all list objects from this list to the end of the given list; clears list
	};

#endif
