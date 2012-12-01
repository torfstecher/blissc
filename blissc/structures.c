//
//  structures.c
//  blissc
//
//  STRUCTURE and FIELD definitions.
//
//  Created by Matthew Madison on 11/28/12.
//  Copyright (c) 2012 Matthew Madison. All rights reserved.
//

#include <stdio.h>
#include "declarations.h"
#include "structures.h"
#include "macros.h"
#include "parser.h"
#include "expression.h"
#include "nametable.h"
#include "lexeme.h"

struct strudef_s {
    strdesc_t *name;
    scopectx_t acctbl, allotbl;
    macparam_t *accformals, *alloformals;
    lexseq_t accbody, allobody;
    int nacc, nallo;
};

static lextype_t delims[] = { LEXTYPE_DELIM_RBRACK, LEXTYPE_DELIM_SEMI };
static lextype_t bodyends[] = { LEXTYPE_DELIM_COMMA, LEXTYPE_DELIM_SEMI };
static strdesc_t leftparen = STRDEF("(");
static strdesc_t rightparen = STRDEF(")");
static strdesc_t dot = STRDEF(".");


/*
 * structure_bind
 */
static int
structure_bind (void *ctx, quotelevel_t ql, quotemodifier_t qm,
                lextype_t lt, condstate_t cs, lexeme_t *lex,
                lexseq_t *result)
{
    parse_ctx_t pctx = ctx;
    name_t *np = lexeme_ctx_get(lex);
    strudef_t *stru = name_data_ptr(np);
    int indecl = parser_in_declaration(pctx);

    if (cs == COND_CWA || cs == COND_AWC) {
        lexeme_free(lex);
        return 1;
    }

    if (indecl) {
        lexeme_type_set(lex, lexeme_boundtype(lex));
        return 0; /* just leave it as is */
    }

    if (!parser_expect(pctx, QL_NORMAL, LEXTYPE_DELIM_LBRACK, 0, 1)) {
        return 0;
    }

    return structure_reference(pctx, stru, 0, lex, result);

} /* structure_bind */

/*
 * structures_init
 */
void
structures_init (void)
{
    lextype_register(LEXTYPE_NAME_STRUCTURE, structure_bind);

} /* structures_init */

strdesc_t *
structure_name (strudef_t *stru)
{
    return stru->name;
}

/*
 * declare_structure
 */
int
declare_structure (parse_ctx_t pctx, scopectx_t scope)
{
    lexeme_t *lex;
    lextype_t term = LEXTYPE_DELIM_COMMA;
    strudef_t *stru;
    name_t *np;
    strdesc_t *text;
    int which;

    while (term == LEXTYPE_DELIM_COMMA) {
        if (!parser_expect(pctx, QL_NAME, LEXTYPE_NAME, &lex, 1)) {
            break;
        }
        text = lexeme_text(lex);
        if (!parser_expect(pctx, QL_NORMAL, LEXTYPE_DELIM_LBRACK, 0, 1)) {
            /* XXX error condition */
        }
        stru = malloc(sizeof(strudef_t));
        memset(stru, 0, sizeof(strudef_t));
        stru->name = string_copy(0, text);
        stru->acctbl = scope_begin(0);
        stru->allotbl = scope_begin(0);
        lexseq_init(&stru->accbody);
        lexseq_init(&stru->allobody);
        which = macro_paramlist(pctx, 0, 1, 0, delims, 2, &stru->acctbl,
                                &stru->accformals, &stru->nacc);
        if (which < 0) {
            /* XXX error condition */
            stru->nacc = 0;
        } else if (which == 1) {
            which = macro_paramlist(pctx, 0, 1, 0, delims, 1,
                                    &stru->allotbl, &stru->alloformals,
                                    &stru->nallo);
            if (which < 0) {
                stru->nallo = 0;
            }
        }
        if (!parser_expect(pctx, QL_NORMAL, LEXTYPE_OP_ASSIGN, 0, 1)) {
            /* XXX error condition */
        }
        if (parser_expect(pctx, QL_NORMAL, LEXTYPE_DELIM_LBRACK, 0, 1)) {
            parser_scope_begin(pctx);
            if (!parse_lexeme_seq(pctx, 0, QL_NAME, delims, 1, &stru->allobody, 0)) {
                /* XXX error condition */
            }
            parser_scope_end(pctx);
        }
        parser_scope_begin(pctx);
        if (!parse_lexeme_seq(pctx, 0, QL_NAME, bodyends, 2, &stru->accbody, &term)) {
            /* XXX error condition */
        }
        parser_scope_end(pctx);
        np = name_declare(scope, text->ptr, text->len, LEXTYPE_NAME_STRUCTURE,
                          parser_curpos(pctx));
        name_data_set_ptr(np, stru);

    } /* while */

    return 1;
    
} /* declare_structure */

/*
 * parse_fields
 *
 * Recursive routine for defining individual fields
 * and field-sets.
 */
static int
parse_fields (parse_ctx_t pctx, scopectx_t scope, lexseq_t *setseq)
{
    strdesc_t *fldname;
    textpos_t fpos;
    lexseq_t fseq;
    lexeme_t *lex;
    name_t *fldnp;
    lextype_t delims[2] = { LEXTYPE_DELIM_COMMA, LEXTYPE_DELIM_RBRACK };
    int which;
    static strdesc_t zero = STRDEF("0");

    while (1) {
        if (!parse_decl_name(pctx, scope, &fldname, &fpos)) {
            /* XXX error condition */
            break;
        }
        if (!parser_expect(pctx, QL_NORMAL, LEXTYPE_OP_ASSIGN, 0, 1)) {
            /* XXX error condition */
        }
        lexseq_init(&fseq);
        // Check for fieldset and recurse
        if (setseq == 0 && parser_expect(pctx, QL_NORMAL, LEXTYPE_KWD_SET, 0, 1)) {
            if (!parse_fields(pctx, scope, &fseq)) {
                /* XXX error condition */
                return 0;
            }
            if (!parser_expect(pctx, QL_NORMAL, LEXTYPE_KWD_TES, 0, 1)) {
                /* XXX error condition */
            }
            fldnp = name_declare(scope, fldname->ptr, fldname->len,
                                 LEXTYPE_NAME_FIELDSET, fpos);
            if (fldnp == 0) {
                /* XXX error condition */
            } else {
                name_data_set_lexseq(fldnp, &fseq);
                lexseq_free(&fseq);
            }
            if (!parser_expect(pctx, QL_NORMAL, LEXTYPE_DELIM_COMMA, 0, 1)) {
                break;
            }
            continue; // set defined; move on to the next name
        }
        // Just a regular field
        if (!parser_expect(pctx, QL_NORMAL, LEXTYPE_DELIM_LBRACK, 0, 1)) {
            /* XXX error condition */
        }
        while (1) {
            if (parse_ctce(pctx, &lex)) {
                lexseq_instail(&fseq, lex);
            } else {
                /* XXX error condition */
                lexseq_instail(&fseq, lexeme_create(LEXTYPE_NUMERIC, &zero));
            }
            which = parser_expect_oneof(pctx, QL_NORMAL, delims, 2, &lex, 1);
            if (which != 0) {
                /* XXX error condition if < 0 */
                break;
            }
            lexseq_instail(&fseq, lex);
        }
        fldnp = name_declare(scope, fldname->ptr, fldname->len,
                             LEXTYPE_NAME_FIELD, fpos);
        if (fldnp == 0) {
            /* XXX error condition */
        } else {
            name_data_set_lexseq(fldnp, &fseq);
            if (setseq != 0) {
                lexseq_instail(setseq, lexeme_create(LEXTYPE_NAME_FIELD, fldname));
            }
        }
        if (!parser_expect(pctx, QL_NORMAL, LEXTYPE_DELIM_COMMA, 0, 1)) {
            break;
        }
    } /* while */

    return 1;

} /* parse_fields */

/*
 * declare_field
 */
int
declare_field (parse_ctx_t pctx, scopectx_t scope)
{
    if (parse_fields(pctx, scope, 0)) {
        return parser_expect(pctx, QL_NORMAL, LEXTYPE_DELIM_SEMI, 0, 1);
    }
    return 0;

} /* declare_field */


/*
 * structure_allocate
 */
int
structure_allocate (parse_ctx_t pctx, name_t *struname,
                    strudef_t **strup, unsigned int *nunits,
                    scopectx_t *scopep)
{
    strudef_t *stru = name_data_ptr(struname);
    machinedef_t *mach = parser_get_machinedef(pctx);
    int i;
    lexeme_t *lex;
    lexseq_t tmpseq;
    scopectx_t myscope, retscope;
    macparam_t *p;
    name_t *np;
    static char *aus[4] = { "BYTE", "WORD", "LONG", "QUAD" };

    if (!parser_expect(pctx, QL_NORMAL, LEXTYPE_DELIM_LBRACK, 0, 1)) {
        *nunits = 0;
        return 1;
    }

    myscope = parser_scope_begin(pctx);
    if (scopep != 0) retscope = scope_begin(0);

    // Predeclare the allocation-unit names to their corresponding
    // values, but only for machines that have 8-bit units, and
    // only those names that correspond to a number of units that
    // is less than or equal to a fullword.
    if (machine_unit_bits(mach) == 8) {
        for (i = 0; i < 4; i++) {
            unsigned int units = 1<<i;
            if (machine_scalar_units(mach) < units) {
                break;
            }
            // Override the reserved-word check here XXX cheat
            np = name_declare_nocheck(myscope, aus[i], strlen(aus[i]), LEXTYPE_NAME_COMPILETIME, 0);
            name_data_set_int(np, units);
        }
    }
    // and the sign-extension keywords, where supported
    if (machine_signext_supported(mach)) {
        np = name_declare_nocheck(myscope, "SIGNED", 6, LEXTYPE_NAME_COMPILETIME, 0);
        name_data_set_int(np, 1);
        np = name_declare_nocheck(myscope, "UNSIGNED", 8, LEXTYPE_NAME_COMPILETIME, 0);
        name_data_set_int(np, 0);
    }
    // Now fill in the default values for the allocation formals, if they
    // have any
    for (p = stru->alloformals; p != 0; p = p->next) {
        strdesc_t *alloname = name_string(p->np);
        lexseq_t *seq;
        np = name_search(stru->allotbl, alloname->ptr, alloname->len, 0);
        seq = name_data_lexseq(np);
        if (lexseq_length(seq) > 0) {
            lexeme_t *lex = lexseq_head(seq);
            if (lexeme_type(lex) == LEXTYPE_NUMERIC) {
                np = name_declare(myscope, alloname->ptr, alloname->len,
                                  LEXTYPE_NAME_COMPILETIME, 0);
                name_data_set_int(np, lexeme_unsignedval(lex));
            }
        }
    }
    for (i = 0, p = stru->alloformals; p != 0 && i < stru->nallo; i++, p = p->next) {
        name_t *rnp;
        strdesc_t *alloname = name_string(p->np);
        np = name_search(myscope, alloname->ptr, alloname->len, 0);
        if (parse_ctce(pctx, &lex)) {
            if (np == 0 || name_scope(np) != myscope) {
                np = name_declare(myscope, alloname->ptr, alloname->len,
                                  LEXTYPE_NAME_COMPILETIME, lexeme_textpos_get(lex));
            }
            name_data_set_int(np, lexeme_signedval(lex));
            lexeme_free(lex);
        } else {
            if (np == 0 || name_scope(np) != myscope) {
                /* XXX error condition - missing param, no default */
                // declare it now to keep things moving
                np = name_declare(myscope, alloname->ptr, alloname->len,
                                  LEXTYPE_NAME_COMPILETIME, parser_curpos(pctx));
            }
        }
        // Now copy the declaration into the scope we'll pass back
        // to the caller for later use
        if (scopep != 0) {
            rnp = name_declare(retscope, alloname->ptr, alloname->len,
                               LEXTYPE_NAME_COMPILETIME, name_dclpos_get(np));
            name_data_set_int(rnp, name_data_int(np));
        }
        if (p->next != 0 &&
            !parser_expect(pctx, QL_NORMAL, LEXTYPE_DELIM_COMMA, 0, 1)) {
            /* XXX error condition */
        }
    }
    if (!parser_expect(pctx, QL_NORMAL, LEXTYPE_DELIM_RBRACK, 0, 1)) {
        /* XXX error condition */
    }
    lexseq_init(&tmpseq);
    lexseq_copy(&tmpseq, &stru->allobody);
    parser_insert_seq(pctx, &tmpseq);
    if (!parse_ctce(pctx, &lex)) {
        /* XXX error condition */
        return 0;
    }
    *nunits = (unsigned int)lexeme_unsignedval(lex);
    lexeme_free(lex);
    if (scopep != 0) *scopep = retscope;
    parser_scope_end(pctx);
    *strup = stru;

    return 1;

} /* structure_allocate */

/*
 * Common structure reference routine
 *
 * General structure reference
 * structure-name [ expression {,access-actual...} {; alloc-actual...} ]
 *
 * Ordinary structure reference
 * segment-name [ access-actual... ]
 *
 */
int
structure_reference (parse_ctx_t pctx, strudef_t *stru,
                     nameinfo_t *ni, lexeme_t *curlex, lexseq_t *result)
{
    scopectx_t curscope = parser_scope_get(pctx);
    lextype_t delim;
    lexseq_t seq;
    scopectx_t myscope, fldscope;
    name_t *np;
    macparam_t *p;
    expr_node_t *exp;
    int which;
    textpos_t pos = lexeme_textpos_get(curlex);
    lexeme_t *lex;
    static lextype_t delims[3] = { LEXTYPE_DELIM_RBRACK, LEXTYPE_DELIM_COMMA,
        LEXTYPE_DELIM_SEMI };
    int ndelims;

    fldscope = 0;
    lexseq_init(&seq);
    // If this is a general structure reference, get the
    // address expression
    if (ni == 0) {
        ndelims = 3;
        if (expr_parse_next(pctx, &lex, 0)) {
            lexseq_instail(&seq, lex);
            which = parser_expect_oneof(pctx, QL_NORMAL, delims, 3, 0, 1);
            if (which < 0) {
                return 0;
            } else {
                delim = delims[which];
            }
        } else if (!parse_lexeme_seq(pctx, 0, QL_NORMAL, delims, 3, &seq, &delim)) {
            return 0;
        }
        myscope = scope_copy(stru->acctbl, 0);
    } else {
        lexeme_t *flex;
        name_t *fnorig, *fncopy;
        if (nameinfo_data_flags(ni) & NI_M_REF) {
            lexseq_instail(&seq, lexeme_create(LEXTYPE_DELIM_LPAR, &leftparen));
            lexseq_instail(&seq, lexeme_create(LEXTYPE_OP_FETCH, &dot));
        }
        lexseq_instail(&seq, lexeme_copy(curlex));
        if (nameinfo_data_flags(ni) & NI_M_REF) {
            lexseq_instail(&seq, lexeme_create(LEXTYPE_DELIM_RPAR, &rightparen));
        }
        // Semicolons (and allocation-formals) not allowed in this case
        ndelims = 2;
        delim = LEXTYPE_DELIM_COMMA;
        myscope = scope_copy(nameinfo_data_struscope(ni), 0);
        // Bring in the field names, so they can be used in the
        // the structure expression
        if (lexseq_length(nameinfo_data_fields(ni)) > 0) {
            fldscope = scope_begin(0);
            for (flex = lexseq_head(nameinfo_data_fields(ni)); flex != 0;
                 flex = lexeme_next(flex)) {
                strdesc_t *fname = lexeme_text(flex);
                fnorig = name_search(curscope, fname->ptr, fname->len, 0);
                if (fnorig != 0 && name_type(fnorig) == LEXTYPE_NAME_FIELD) {
                    fncopy = name_declare(fldscope, fname->ptr, fname->len, LEXTYPE_NAME_MAC_PARAM, lexeme_textpos_get(curlex));
                    if (fncopy != 0) {
                        lexseq_copy(name_data_lexseq(fncopy), name_data_lexseq(fnorig));
                    }
                }
            }
        }
    }
    np = name_declare(myscope, stru->name->ptr, stru->name->len,
                      LEXTYPE_NAME_MAC_PARAM, pos);
    name_data_set_lexseq(np, &seq);
    if (fldscope != 0) {
        parser_scope_set(pctx, fldscope);
    }
    for (p = stru->accformals; p != 0; p = p->next) {
        strdesc_t *pname = name_string(p->np);
        lexseq_init(&seq);
        if (delim == LEXTYPE_DELIM_COMMA) {
            if (expr_parse_next(pctx, &lex, 0)) {
                lexseq_instail(&seq, lex);
                which = parser_expect_oneof(pctx, QL_NORMAL, delims, ndelims, 0, 1);
                if (which < 0) {
                    return 0;
                } else {
                    delim = delims[which];
                }
            } else if (!parse_lexeme_seq(pctx, 0, QL_NORMAL, delims, ndelims, &seq, &delim)) {
                break;
            }
        }
        if (lexseq_length(&seq) == 0) {
            name_t *ap = name_search(stru->acctbl, pname->ptr, pname->len, 0);
            if (ap != 0) {
                lexseq_copy(&seq, name_data_lexseq(ap));
            }
        }
        np = name_declare(myscope, pname->ptr, pname->len, LEXTYPE_NAME_MAC_PARAM, pos);
        name_data_set_lexseq(np, &seq);
    }
    if (fldscope != 0) {
        parser_scope_end(pctx);
    }
    // Allocation-formals are only used in the general-reference case
    if (ni == 0) {
        for (p = stru->alloformals; p != 0; p = p->next) {
            strdesc_t *pname = name_string(p->np);
            lexseq_init(&seq);
            if (delim != LEXTYPE_DELIM_RBRACK) {
                if (expr_parse_next(pctx, &lex, 0)) {
                    lexseq_instail(&seq, lex);
                    which = parser_expect_oneof(pctx, QL_NORMAL, delims, 3, 0, 1);
                    if (which < 0) {
                        return 0;
                    } else {
                        delim = delims[which];
                    }
                } else if (!parse_lexeme_seq(pctx, 0, QL_NORMAL, delims, 3, &seq, &delim)) {
                    break;
                }
            }
            if (lexseq_length(&seq) == 0) {
                name_t *ap = name_search(stru->allotbl, pname->ptr, pname->len, 0);
                if (ap != 0) {
                    lexseq_copy(&seq, name_data_lexseq(ap));
                }
            }
            np = name_declare(myscope, pname->ptr, pname->len, LEXTYPE_NAME_MAC_PARAM, pos);
            name_data_set_lexseq(np, &seq);
        }
    }
    if (delim != LEXTYPE_DELIM_RBRACK) {
        /* XXX error condition */
        parser_skip_to_delim(pctx, LEXTYPE_DELIM_RBRACK);
    }
    // Enclose the result in parentheses so it is treated as a
    // field-referenceable primary.
    lexseq_init(&seq);
    lexseq_inshead(&seq, lexeme_create(LEXTYPE_DELIM_LPAR, &leftparen));
    lexseq_copy(&seq, &stru->accbody);
    lexseq_instail(&seq, lexeme_create(LEXTYPE_DELIM_RPAR, &rightparen));
    parser_scope_set(pctx, myscope);
    if (!expr_parse_seq(pctx, &seq, &exp)) {
        /* XXX error condition */
        lexseq_free(&seq);
        parser_scope_end(pctx);
        return -1;
    }

    parser_scope_end(pctx);
    if (expr_setlex(pctx, &curlex, exp)) {
        return 0;
    }
    return -1;

} /* structure_reference */