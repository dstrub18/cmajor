/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include "cghdr.h"

static Agraph_t *agfindsubg_by_id(Agraph_t * g, IDTYPE id)
{
    Agraph_t template_;

    agdtdisc(g, g->g_dict, &Ag_subgraph_id_disc);
    AGID(&template_) = id;
    return (Agraph_t*) dtsearch(g->g_dict, &template_);
}

static Agraph_t *localsubg(Agraph_t * g, IDTYPE id)
{
    Agraph_t *subg;

    subg = agfindsubg_by_id(g, id);
    if (subg)
	return subg;

    subg = (Agraph_t*) agalloc(g, sizeof(Agraph_t));
    subg->clos = g->clos;
    subg->desc = g->desc;
    subg->desc.maingraph = FALSE;
    subg->parent = g;
    subg->root = g->root;
    AGID(subg) = id;
    return agopen1(subg);
}

Agraph_t *agidsubg(Agraph_t * g, IDTYPE id, int cflag)
{
    Agraph_t *subg;
    subg = agfindsubg_by_id(g, id);
    if (subg == NULL && cflag && agallocid(g, AGRAPH, id))
	subg = localsubg(g, id);
    return subg;
}

Agraph_t *agsubg(Agraph_t * g, char *name, int cflag)
{
    IDTYPE id;
    Agraph_t *subg;

    if (name && agmapnametoid(g, AGRAPH, name, &id, FALSE)) {
	/* might already exist */
	if ((subg = agfindsubg_by_id(g, id)))
	    return subg;
    }

    if (cflag && agmapnametoid(g, AGRAPH, name, &id, TRUE)) {	/* reserve id */
	subg = localsubg(g, id);
	agregister(g, AGRAPH, subg);
	return subg;
    }

    return NULL;
}

Agraph_t *agfstsubg(Agraph_t * g)
{
    return (Agraph_t*) dtfirst(g->g_dict);
}

Agraph_t *agnxtsubg(Agraph_t * subg)
{
    Agraph_t *g;

    g = agparent(subg);
    return g? (Agraph_t*)dtnext(g->g_dict, subg) : 0;
}

Agraph_t *agparent(Agraph_t * g)
{
    return g->parent;
}

/* this function is only responsible for deleting the entry
 * in the parent's subg dict.  the rest is done in agclose().
 */
int agdelsubg(Agraph_t * g, Agraph_t * subg)
{
    return dtdelete(g->g_dict, subg) != NULL;
}
