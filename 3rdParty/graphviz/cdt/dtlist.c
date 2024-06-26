
#include	"dthdr.h"
#include	<stddef.h>

/*	List, Deque, Stack, Queue.
**
**	Written by Kiem-Phong Vo (05/25/96)
*/

static void* dtlist(Dt_t* dt, void* obj, int type)
{
	int		lk, sz, ky;
	Dtcompar_f	cmpf;
	Dtdisc_t*	disc;
	Dtlink_t	*r, *t;
	void	*key, *k;

	UNFLATTEN(dt);
	disc = dt->disc; _DTDSC(disc,ky,sz,lk,cmpf);
	dt->type &= ~DT_FOUND;

	if(!obj)
	{	if(type&(DT_LAST|DT_FIRST) )
		{	if((r = dt->data->head) )
			{	if(type&DT_LAST)
					r = r->left;
				dt->data->here = r;
			}
			return r ? _DTOBJ(r,lk) : NULL;
		}
		else if(type&(DT_DELETE|DT_DETACH))
		{	if((dt->data->type&(DT_LIST|DT_DEQUE)) || !(r = dt->data->head))
				return NULL;
			else	goto dt_delete;
		}
		else if(type&DT_CLEAR)
		{	if(disc->freef || disc->link < 0)
			{	for(r = dt->data->head; r; r = t)
				{	t = r->right;
					if(disc->freef)
						disc->freef(dt, _DTOBJ(r, lk), disc);
					if(disc->link < 0)
						dt->memoryf(dt, r, 0, disc);
				}
			}
			dt->data->head = dt->data->here = NULL;
			dt->data->size = 0;
			return NULL;
		}
		else	return NULL;
	}

	if(type&(DT_INSERT|DT_ATTACH))
	{	if (disc->makef && (type&DT_INSERT) && !(obj = disc->makef(dt, obj, disc)))
			return NULL;
		if(lk >= 0)
			r = _DTLNK(obj,lk);
		else
		{	r = (Dtlink_t*)dt->memoryf(dt, NULL, sizeof(Dthold_t), disc);
			if(r)
				((Dthold_t*)r)->obj = obj;
			else
			{	if(disc->makef && disc->freef && (type&DT_INSERT))
					disc->freef(dt, obj, disc);
				return NULL;
			}
		}

		if(dt->data->type&DT_DEQUE)
		{	if(type&DT_APPEND)
				goto dt_queue;
			else	goto dt_stack;
		}
		else if(dt->data->type&DT_LIST)
		{	if(type&DT_APPEND)
			{	if(!(t = dt->data->here) || !t->right)
					goto dt_queue;
				r->right = t->right;
				r->right->left = r;
				r->left = t;
				r->left->right = r;
			}
			else
			{	if(!(t = dt->data->here) || t == dt->data->head)
					goto dt_stack;
				r->left = t->left;
				r->left->right = r;
				r->right = t;
				r->right->left = r;
			}
		}
		else if(dt->data->type&DT_STACK)
		{ dt_stack:
			r->right = t = dt->data->head;
			if(t)
			{	r->left = t->left;
				t->left = r;
			}
			else	r->left = r;
			dt->data->head = r;
		}
		else /* if(dt->data->type&DT_QUEUE) */
		{ dt_queue:
			if((t = dt->data->head) )
			{	t->left->right = r;
				r->left = t->left;
				t->left = r;
			}
			else
			{	dt->data->head = r;
				r->left = r;
			}
			r->right = NULL;
		}

		if(dt->data->size >= 0)
			dt->data->size += 1;

		dt->data->here = r;
		return _DTOBJ(r,lk);
	}

	if((type&DT_MATCH) || !(r = dt->data->here) || _DTOBJ(r,lk) != obj)
	{	key = (type&DT_MATCH) ? obj : _DTKEY(obj,ky,sz);
		for(r = dt->data->head; r; r = r->right)
		{	k = _DTOBJ(r,lk); k = _DTKEY(k,ky,sz);
			if(_DTCMP(dt,key,k,disc,cmpf,sz) == 0)
				break;
		}
	}

	if(!r)
		return NULL;
	dt->type |= DT_FOUND;

	if(type&(DT_DELETE|DT_DETACH))
	{ dt_delete:
		if(r->right)
			r->right->left = r->left;
		if(r == (t = dt->data->head) )
		{	dt->data->head = r->right;
			if(dt->data->head)
				dt->data->head->left = t->left;
		}
		else
		{	r->left->right = r->right;
			if(r == t->left)
				t->left = r->left;
		}

		dt->data->here = r == dt->data->here ? r->right : NULL;
		dt->data->size -= 1;

		obj = _DTOBJ(r,lk);
		if(disc->freef && (type&DT_DELETE))
			disc->freef(dt, obj, disc);
		if(disc->link < 0)
			dt->memoryf(dt, r, 0, disc);
		return obj;
	}
	else if(type&DT_NEXT)
		r = r->right;
	else if(type&DT_PREV)
		r = r == dt->data->head ? NULL : r->left;

	dt->data->here = r;
	return r ? _DTOBJ(r,lk) : NULL;
}

Dtmethod_t _Dtlist  = { dtlist, DT_LIST  };
Dtmethod_t _Dtdeque  = { dtlist, DT_DEQUE  };
Dtmethod_t _Dtstack = { dtlist, DT_STACK };
Dtmethod_t _Dtqueue = { dtlist, DT_QUEUE };

Dtmethod_t* Dtlist = &_Dtlist;
Dtmethod_t* Dtdeque = &_Dtdeque;
Dtmethod_t* Dtstack = &_Dtstack;
Dtmethod_t* Dtqueue = &_Dtqueue;
