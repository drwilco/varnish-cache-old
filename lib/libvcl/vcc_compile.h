/*-
 * Copyright (c) 2006 Verdens Gang AS
 * Copyright (c) 2006-2009 Linpro AS
 * All rights reserved.
 *
 * Author: Poul-Henning Kamp <phk@phk.freebsd.dk>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

#include "vqueue.h"

#include "vcl.h"

#define INDENT		2

struct acl_e;

struct membit {
	VTAILQ_ENTRY(membit)	list;
	void			*ptr;
};

struct source {
	VTAILQ_ENTRY(source)	list;
	char			*name;
	const char		*b;
	const char		*e;
	unsigned		idx;
	char			*freeit;
};

struct token {
	unsigned		tok;
	const char		*b;
	const char		*e;
	struct source		*src;
	VTAILQ_ENTRY(token)	list;
	unsigned		cnt;
	char			*dec;
};

struct fini {
	VTAILQ_ENTRY(fini)	list;
	char			*text;
};

VTAILQ_HEAD(tokenhead, token);

struct tokenlist {
	struct tokenhead	tokens;
	VTAILQ_HEAD(, source)	sources;
	VTAILQ_HEAD(, membit)	membits;
	unsigned		nsources;
	struct source		*src;
	struct token		*t;
	int			indent;
	int			hindent;
	int			iindent;
	int			findent;
	unsigned		cnt;
	struct vsb		*fc, *fh, *fi, *ff, *fb;
	struct vsb		*fm[VCL_MET_MAX];
	VTAILQ_HEAD(, ref)	refs;
	VTAILQ_HEAD(, fini)	finis;
	struct vsb		*sb;
	int			err;
	int			ndirector;
	VTAILQ_HEAD(, proc)	procs;
	struct proc		*curproc;
	struct proc		*mprocs[VCL_MET_MAX];

	VTAILQ_HEAD(, acl_e)	acl;

	struct token		*t_dir;
	struct token		*t_policy;

	unsigned		recnt;
	unsigned		nsockaddr;
};

enum var_type {
	BACKEND,
	BOOL,
	INT,
	FLOAT,
	SIZE,
	TIME,
	RTIME,
	STRING,
	IP,
	HASH,
	HEADER
};

enum var_access {
	V_RO,
	V_RW,
	V_WO
};

enum ref_type {
	R_FUNC,
	R_ACL,
	R_BACKEND
};

struct ref {
	enum ref_type		type;
	struct token		*name;
	unsigned		defcnt;
	unsigned		refcnt;
	VTAILQ_ENTRY(ref)	list;
};

struct var {
	const char		*name;
	enum var_type		fmt;
	unsigned		len;
	const char		*rname;
	const char		*lname;
	enum var_access		access;
	const char		*hdr;
	unsigned		methods;
};

struct method {
	const char		*name;
	unsigned		ret_bitmap;
	unsigned		bitval;
};

/*--------------------------------------------------------------------*/

/* vcc_acl.c */

void vcc_Acl(struct tokenlist *tl);
void vcc_Cond_Ip(const struct var *vp, struct tokenlist *tl);

/* vcc_action.c */
int vcc_ParseAction(struct tokenlist *tl);

/* vcc_backend.c */
struct fld_spec;
typedef void parsedirector_f(struct tokenlist *tl);

void vcc_ParseDirector(struct tokenlist *tl);
void vcc_ParseBackendHost(struct tokenlist *tl, int serial, char **nm);
struct fld_spec * vcc_FldSpec(struct tokenlist *tl, const char *first, ...);
void vcc_ResetFldSpec(struct fld_spec *f);
void vcc_IsField(struct tokenlist *tl, struct token **t, struct fld_spec *fs);
void vcc_FieldsOk(struct tokenlist *tl, const struct fld_spec *fs);

void Emit_Sockaddr(struct tokenlist *tl, const struct token *t_host, const char *port);
void vcc_EmitBeIdent(const struct tokenlist *tl, struct vsb *v,
	 int serial, const struct token *first, const struct token *last);

/* vcc_compile.c */
extern struct method method_tab[];
/*
 * H -> Header, before the C code
 * C -> C-code
 * B -> Body of function, ends up in C once function is completed
 * I -> Initializer function
 * F -> Finish function
 */
void Fh(const struct tokenlist *tl, int indent, const char *fmt, ...);
void Fc(const struct tokenlist *tl, int indent, const char *fmt, ...);
void Fb(const struct tokenlist *tl, int indent, const char *fmt, ...);
void Fi(const struct tokenlist *tl, int indent, const char *fmt, ...);
void Ff(const struct tokenlist *tl, int indent, const char *fmt, ...);
void EncToken(struct vsb *sb, const struct token *t);
int IsMethod(const struct token *t);
void *TlAlloc(struct tokenlist *tl, unsigned len);

/* vcc_dir_random.c */
parsedirector_f vcc_ParseRandomDirector;

/* vcc_dir_round_robin.c */
parsedirector_f vcc_ParseRoundRobinDirector;

/* vcc_dir_dns.c */
parsedirector_f vcc_ParseDnsDirector;

/* vcc_obj.c */
extern struct var vcc_vars[];

/* vcc_parse.c */
void vcc_Parse(struct tokenlist *tl);
void vcc_RTimeVal(struct tokenlist *tl, double *);
void vcc_TimeVal(struct tokenlist *tl, double *);
void vcc_SizeVal(struct tokenlist *tl, double *);
unsigned vcc_UintVal(struct tokenlist *tl);
double vcc_DoubleVal(struct tokenlist *tl);

/* vcc_string.c */
char *vcc_regexp(struct tokenlist *tl);
int vcc_StringVal(struct tokenlist *tl);
void vcc_ExpectedStringval(struct tokenlist *tl);

/* vcc_token.c */
void vcc_Coord(const struct tokenlist *tl, struct vsb *vsb,
    const struct token *t);
void vcc_ErrToken(const struct tokenlist *tl, const struct token *t);
void vcc_ErrWhere(struct tokenlist *tl, const struct token *t);
void vcc__Expect(struct tokenlist *tl, unsigned tok, int line);
int vcc_Teq(const struct token *t1, const struct token *t2);
int vcc_IdIs(const struct token *t, const char *p);
void vcc_ExpectCid(struct tokenlist *tl);
void vcc_Lexer(struct tokenlist *tl, struct source *sp);
void vcc_NextToken(struct tokenlist *tl);
void vcc__ErrInternal(struct tokenlist *tl, const char *func,
    unsigned line);
void vcc_AddToken(struct tokenlist *tl, unsigned tok, const char *b,
    const char *e);

/* vcc_var.c */
struct var *vcc_FindVar(struct tokenlist *tl, const struct token *t,
    struct var *vl);
void vcc_VarVal(struct tokenlist *tl, const struct var *vp,
    const struct token *vt);

/* vcc_xref.c */
struct ref *vcc_FindRef(struct tokenlist *tl, struct token *t, enum ref_type type);
void vcc_AddDef(struct tokenlist *tl, struct token *t, enum ref_type type);
void vcc_AddRef(struct tokenlist *tl, struct token *t, enum ref_type type);
int vcc_CheckReferences(struct tokenlist *tl);

void vcc_AddCall(struct tokenlist *tl, struct token *t);
struct proc *vcc_AddProc(struct tokenlist *tl, struct token *t);
void vcc_ProcAction(struct proc *p, unsigned action, struct token *t);
int vcc_CheckAction(struct tokenlist *tl);
void vcc_AddUses(struct tokenlist *tl, struct var *v);
int vcc_CheckUses(struct tokenlist *tl);

#define ERRCHK(tl)      do { if ((tl)->err) return; } while (0)
#define ErrInternal(tl) vcc__ErrInternal(tl, __func__, __LINE__)
#define Expect(a, b) vcc__Expect(a, b, __LINE__)
#define ExpectErr(a, b) \
    do { vcc__Expect(a, b, __LINE__); ERRCHK(a);} while (0)
#define SkipToken(a, b) \
    do { vcc__Expect(a, b, __LINE__); ERRCHK(a); vcc_NextToken(a); } while (0)
