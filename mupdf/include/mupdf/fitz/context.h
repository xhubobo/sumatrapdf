#ifndef MUPDF_FITZ_CONTEXT_H
#define MUPDF_FITZ_CONTEXT_H

#include "mupdf/fitz/version.h"
#include "mupdf/fitz/system.h"
#include "mupdf/fitz/geometry.h"

typedef struct fz_font_context_s fz_font_context;
typedef struct fz_colorspace_context_s fz_colorspace_context;
typedef struct fz_style_context_s fz_style_context;
typedef struct fz_tuning_context_s fz_tuning_context;
typedef struct fz_store_s fz_store;
typedef struct fz_glyph_cache_s fz_glyph_cache;
typedef struct fz_document_handler_context_s fz_document_handler_context;
typedef struct fz_context_s fz_context;

typedef struct fz_alloc_context_s fz_alloc_context;
struct fz_alloc_context_s
{
	void *user;
	void *(*malloc)(void *, size_t);
	void *(*realloc)(void *, void *, size_t);
	void (*free)(void *, void *);
};

typedef struct fz_error_stack_slot_s fz_error_stack_slot;
struct fz_error_stack_slot_s
{
	int state, code;
	fz_jmp_buf buffer;
};

typedef struct fz_error_context_s fz_error_context;
struct fz_error_context_s
{
	fz_error_stack_slot *top;
	fz_error_stack_slot stack[256];
	int errcode;
	void *print_user;
	void (*print)(void *user, const char *message);
	char message[256];
};

typedef struct fz_warn_context_s fz_warn_context;
struct fz_warn_context_s
{
	void *print_user;
	void (*print)(void *user, const char *message);
	int count;
	char message[256];
};

typedef struct fz_aa_context_s fz_aa_context;
struct fz_aa_context_s
{
	int hscale;
	int vscale;
	int scale;
	int bits;
	int text_bits;
	float min_line_width;
};

/*
	Exception macro definitions. Just treat these as a black box -
	pay no attention to the man behind the curtain.
*/

void fz_var_imp(void *);
#define fz_var(var) fz_var_imp((void *)&(var))

fz_jmp_buf *fz_push_try(fz_context *ctx);
int fz_do_try(fz_context *ctx);
int fz_do_always(fz_context *ctx);
int fz_do_catch(fz_context *ctx);

#define fz_try(ctx) if (!fz_setjmp(*fz_push_try(ctx))) if (fz_do_try(ctx)) do
#define fz_always(ctx) while (0); if (fz_do_always(ctx)) do
#define fz_catch(ctx) while (0); if (fz_do_catch(ctx))

FZ_NORETURN void fz_vthrow(fz_context *ctx, int errcode, const char *, va_list ap);
FZ_NORETURN void fz_throw(fz_context *ctx, int errcode, const char *, ...) FZ_PRINTFLIKE(3,4);
FZ_NORETURN void fz_rethrow(fz_context *ctx);
void fz_vwarn(fz_context *ctx, const char *fmt, va_list ap);
void fz_warn(fz_context *ctx, const char *fmt, ...) FZ_PRINTFLIKE(2,3);
const char *fz_caught_message(fz_context *ctx);
int fz_caught(fz_context *ctx);
void fz_rethrow_if(fz_context *ctx, int errcode);

enum
{
	FZ_ERROR_NONE = 0,
	FZ_ERROR_MEMORY = 1,
	FZ_ERROR_GENERIC = 2,
	FZ_ERROR_SYNTAX = 3,
	FZ_ERROR_MINOR = 4,
	FZ_ERROR_TRYLATER = 5,
	FZ_ERROR_ABORT = 6,
	FZ_ERROR_COUNT
};

/*
	Flush any repeated warnings.

	Repeated warnings are buffered, counted and eventually printed
	along with the number of repetitions. Call fz_flush_warnings
	to force printing of the latest buffered warning and the
	number of repetitions, for example to make sure that all
	warnings are printed before exiting an application.
*/
void fz_flush_warnings(fz_context *ctx);

/*
	Locking functions

	MuPDF is kept deliberately free of any knowledge of particular
	threading systems. As such, in order for safe multi-threaded
	operation, we rely on callbacks to client provided functions.

	A client is expected to provide FZ_LOCK_MAX number of mutexes,
	and a function to lock/unlock each of them. These may be
	recursive mutexes, but do not have to be.

	If a client does not intend to use multiple threads, then it
	may pass NULL instead of a lock structure.

	In order to avoid deadlocks, we have one simple rule
	internally as to how we use locks: We can never take lock n
	when we already hold any lock i, where 0 <= i <= n. In order
	to verify this, we have some debugging code, that can be
	enabled by defining FITZ_DEBUG_LOCKING.
*/

typedef struct fz_locks_context_s fz_locks_context;
struct fz_locks_context_s
{
	void *user;
	void (*lock)(void *user, int lock);
	void (*unlock)(void *user, int lock);
};

enum {
	FZ_LOCK_ALLOC = 0,
	FZ_LOCK_FREETYPE,
	FZ_LOCK_GLYPHCACHE,
	FZ_LOCK_MAX
};

#if defined(MEMENTO) || !defined(NDEBUG)
#define FITZ_DEBUG_LOCKING
#endif

#ifdef FITZ_DEBUG_LOCKING

void fz_assert_lock_held(fz_context *ctx, int lock);
void fz_assert_lock_not_held(fz_context *ctx, int lock);
void fz_lock_debug_lock(fz_context *ctx, int lock);
void fz_lock_debug_unlock(fz_context *ctx, int lock);

#else

#define fz_assert_lock_held(A,B) do { } while (0)
#define fz_assert_lock_not_held(A,B) do { } while (0)
#define fz_lock_debug_lock(A,B) do { } while (0)
#define fz_lock_debug_unlock(A,B) do { } while (0)

#endif /* !FITZ_DEBUG_LOCKING */

struct fz_context_s
{
	void *user;
	fz_alloc_context alloc;
	fz_locks_context locks;
	fz_error_context error;
	fz_warn_context warn;

	/* unshared contexts */
	fz_aa_context aa;
	uint16_t seed48[7];
#if FZ_ENABLE_ICC
	int icc_enabled;
#endif

	/* TODO: should these be unshared? */
	fz_document_handler_context *handler;
	fz_style_context *style;
	fz_tuning_context *tuning;

	/* shared contexts */
	fz_font_context *font;
	fz_colorspace_context *colorspace;
	fz_store *store;
	fz_glyph_cache *glyph_cache;
};

/*
	Specifies the maximum size in bytes of the resource store in
	fz_context. Given as argument to fz_new_context.

	FZ_STORE_UNLIMITED: Let resource store grow unbounded.

	FZ_STORE_DEFAULT: A reasonable upper bound on the size, for
	devices that are not memory constrained.
*/
enum {
	FZ_STORE_UNLIMITED = 0,
	FZ_STORE_DEFAULT = 256 << 20,
};

/*
	Allocate context containing global state.

	The global state contains an exception stack, resource store,
	etc. Most functions in MuPDF take a context argument to be
	able to reference the global state. See fz_drop_context for
	freeing an allocated context.

	alloc: Supply a custom memory allocator through a set of
	function pointers. Set to NULL for the standard library
	allocator. The context will keep the allocator pointer, so the
	data it points to must not be modified or freed during the
	lifetime of the context.

	locks: Supply a set of locks and functions to lock/unlock
	them, intended for multi-threaded applications. Set to NULL
	when using MuPDF in a single-threaded applications. The
	context will keep the locks pointer, so the data it points to
	must not be modified or freed during the lifetime of the
	context.

	max_store: Maximum size in bytes of the resource store, before
	it will start evicting cached resources such as fonts and
	images. FZ_STORE_UNLIMITED can be used if a hard limit is not
	desired. Use FZ_STORE_DEFAULT to get a reasonable size.

	May return NULL.
*/
fz_context *fz_new_context_imp(const fz_alloc_context *alloc, const fz_locks_context *locks, size_t max_store, const char *version);

#define fz_new_context(alloc, locks, max_store) fz_new_context_imp(alloc, locks, max_store, FZ_VERSION)

/*
	Make a clone of an existing context.

	This function is meant to be used in multi-threaded
	applications where each thread requires its own context, yet
	parts of the global state, for example caching, are shared.

	ctx: Context obtained from fz_new_context to make a copy of.
	ctx must have had locks and lock/functions setup when created.
	The two contexts will share the memory allocator, resource
	store, locks and lock/unlock functions. They will each have
	their own exception stacks though.

	May return NULL.
*/
fz_context *fz_clone_context(fz_context *ctx);

/*
	Free a context and its global state.

	The context and all of its global state is freed, and any
	buffered warnings are flushed (see fz_flush_warnings). If NULL
	is passed in nothing will happen.
*/
void fz_drop_context(fz_context *ctx);

/*
	Set the user field in the context.

	NULL initially, this field can be set to any opaque value
	required by the user. It is copied on clones.
*/
void fz_set_user_context(fz_context *ctx, void *user);

/*
	Read the user field from the context.
*/
void *fz_user_context(fz_context *ctx);

void fz_default_error_callback(void *user, const char *message);
void fz_default_warning_callback(void *user, const char *message);

void fz_set_error_callback(fz_context *ctx, void (*print)(void *user, const char *message), void *user);
void fz_set_warning_callback(fz_context *ctx, void (*print)(void *user, const char *message), void *user);

static inline void
fz_lock(fz_context *ctx, int lock)
{
	fz_lock_debug_lock(ctx, lock);
	ctx->locks.lock(ctx->locks.user, lock);
}

static inline void
fz_unlock(fz_context *ctx, int lock)
{
	fz_lock_debug_unlock(ctx, lock);
	ctx->locks.unlock(ctx->locks.user, lock);
}

/*
	In order to tune MuPDF's behaviour, certain functions can
	(optionally) be provided by callers.
*/

/*
	Given the width and height of an image,
	the subsample factor, and the subarea of the image actually
	required, the caller can decide whether to decode the whole
	image or just a subarea.

	arg: The caller supplied opaque argument.

	w, h: The width/height of the complete image.

	l2factor: The log2 factor for subsampling (i.e. image will be
	decoded to (w>>l2factor, h>>l2factor)).

	subarea: The actual subarea required for the current operation.
	The tuning function is allowed to increase this in size if
	required.
*/
typedef void (fz_tune_image_decode_fn)(void *arg, int w, int h, int l2factor, fz_irect *subarea);

/*
	Given the source width and height of
	image, together with the actual required width and height,
	decide whether we should use mitchell scaling.

	arg: The caller supplied opaque argument.

	dst_w, dst_h: The actual width/height required on the target
	device.

	src_w, src_h: The source width/height of the image.

	Return 0 not to use the Mitchell scaler, 1 to use the Mitchell
	scaler. All other values reserved.
*/
typedef int (fz_tune_image_scale_fn)(void *arg, int dst_w, int dst_h, int src_w, int src_h);

/*
	Set the tuning function to use for
	image decode.

	image_decode: Function to use.

	arg: Opaque argument to be passed to tuning function.
*/
void fz_tune_image_decode(fz_context *ctx, fz_tune_image_decode_fn *image_decode, void *arg);

/*
	Set the tuning function to use for
	image scaling.

	image_scale: Function to use.

	arg: Opaque argument to be passed to tuning function.
*/
void fz_tune_image_scale(fz_context *ctx, fz_tune_image_scale_fn *image_scale, void *arg);

/*
	Get the number of bits of antialiasing we are
	using (for graphics). Between 0 and 8.
*/
int fz_aa_level(fz_context *ctx);

/*
	Set the number of bits of antialiasing we should
	use (for both text and graphics).

	bits: The number of bits of antialiasing to use (values are
	clamped to within the 0 to 8 range).
*/
void fz_set_aa_level(fz_context *ctx, int bits);

/*
	Get the number of bits of antialiasing we are
	using for text. Between 0 and 8.
*/
int fz_text_aa_level(fz_context *ctx);

/*
	Set the number of bits of antialiasing we
	should use for text.

	bits: The number of bits of antialiasing to use (values are
	clamped to within the 0 to 8 range).
*/
void fz_set_text_aa_level(fz_context *ctx, int bits);

/*
	Get the number of bits of antialiasing we are
	using for graphics. Between 0 and 8.
*/
int fz_graphics_aa_level(fz_context *ctx);

/*
	Set the number of bits of antialiasing we
	should use for graphics.

	bits: The number of bits of antialiasing to use (values are
	clamped to within the 0 to 8 range).
*/
void fz_set_graphics_aa_level(fz_context *ctx, int bits);

/*
	Get the minimum line width to be
	used for stroked lines.

	min_line_width: The minimum line width to use (in pixels).
*/
float fz_graphics_min_line_width(fz_context *ctx);

/*
	Set the minimum line width to be
	used for stroked lines.

	min_line_width: The minimum line width to use (in pixels).
*/
void fz_set_graphics_min_line_width(fz_context *ctx, float min_line_width);

/*
	Get the user stylesheet source text.
*/
const char *fz_user_css(fz_context *ctx);

/*
	Set the user stylesheet source text for use with HTML and EPUB.
*/
void fz_set_user_css(fz_context *ctx, const char *text);

/*
	Return whether to respect document styles in HTML and EPUB.
*/
int fz_use_document_css(fz_context *ctx);

/*
	Toggle whether to respect document styles in HTML and EPUB.
*/
void fz_set_use_document_css(fz_context *ctx, int use);

void fz_enable_icc(fz_context *ctx);
void fz_disable_icc(fz_context *ctx);

/*
	Memory Allocation and Scavenging:

	All calls to MuPDF's allocator functions pass through to the
	underlying allocators passed in when the initial context is
	created, after locks are taken (using the supplied locking
	function) to ensure that only one thread at a time calls
	through.

	If the underlying allocator fails, MuPDF attempts to make room
	for the allocation by evicting elements from the store, then
	retrying.

	Any call to allocate may then result in several calls to the
	underlying allocator, and result in elements that are only
	referred to by the store being freed.
*/

/*
 * Allocate memory for a structure, clear it, and tag the pointer for
 * Memento.
 */
#define fz_malloc_struct(CTX, TYPE) \
	((TYPE*)Memento_label(fz_calloc(CTX, 1, sizeof(TYPE)), #TYPE))

/*
 * Allocate uninitialized memory for an array of structures, and tag the
 * pointer for Memento. Does NOT clear the memory!
 */
#define fz_malloc_array(CTX, COUNT, TYPE) \
	((TYPE*)Memento_label(fz_malloc(CTX, (COUNT) * sizeof(TYPE)), #TYPE "[]"))
#define fz_realloc_array(CTX, OLD, COUNT, TYPE) \
	((TYPE*)Memento_label(fz_realloc(CTX, OLD, (COUNT) * sizeof(TYPE)), #TYPE "[]"))

void *fz_malloc(fz_context *ctx, size_t size);
void *fz_calloc(fz_context *ctx, size_t count, size_t size);
void *fz_realloc(fz_context *ctx, void *p, size_t size);
void fz_free(fz_context *ctx, void *p);

void *fz_malloc_no_throw(fz_context *ctx, size_t size);
void *fz_calloc_no_throw(fz_context *ctx, size_t count, size_t size);
void *fz_realloc_no_throw(fz_context *ctx, void *p, size_t size);

char *fz_strdup(fz_context *ctx, const char *s);

void *fz_zlib_alloc(void *ctx, unsigned int items, unsigned int size);
void fz_zlib_free(void *ctx, void *ptr);

extern fz_alloc_context fz_alloc_default;
extern fz_locks_context fz_locks_default;

double fz_drand48(fz_context *ctx);
int32_t fz_lrand48(fz_context *ctx);
int32_t fz_mrand48(fz_context *ctx);
double fz_erand48(fz_context *ctx, uint16_t xsubi[3]);
int32_t fz_jrand48(fz_context *ctx, uint16_t xsubi[3]);
int32_t fz_nrand48(fz_context *ctx, uint16_t xsubi[3]);
void fz_lcong48(fz_context *ctx, uint16_t param[7]);
uint16_t *fz_seed48(fz_context *ctx, uint16_t seed16v[3]);
void fz_srand48(fz_context *ctx, int32_t seedval);

/*
	Fill block with len bytes of pseudo-randomness.
*/
void fz_memrnd(fz_context *ctx, uint8_t *block, int len);

/* Lock-safe reference counting functions */

static inline void *
fz_keep_imp(fz_context *ctx, void *p, int *refs)
{
	if (p)
	{
		(void)Memento_checkIntPointerOrNull(refs);
		fz_lock(ctx, FZ_LOCK_ALLOC);
		if (*refs > 0)
		{
			(void)Memento_takeRef(p);
			++*refs;
		}
		fz_unlock(ctx, FZ_LOCK_ALLOC);
	}
	return p;
}

static inline void *
fz_keep_imp8(fz_context *ctx, void *p, int8_t *refs)
{
	if (p)
	{
		(void)Memento_checkBytePointerOrNull(refs);
		fz_lock(ctx, FZ_LOCK_ALLOC);
		if (*refs > 0)
		{
			(void)Memento_takeRef(p);
			++*refs;
		}
		fz_unlock(ctx, FZ_LOCK_ALLOC);
	}
	return p;
}

static inline void *
fz_keep_imp16(fz_context *ctx, void *p, int16_t *refs)
{
	if (p)
	{
		(void)Memento_checkShortPointerOrNull(refs);
		fz_lock(ctx, FZ_LOCK_ALLOC);
		if (*refs > 0)
		{
			(void)Memento_takeRef(p);
			++*refs;
		}
		fz_unlock(ctx, FZ_LOCK_ALLOC);
	}
	return p;
}

static inline int
fz_drop_imp(fz_context *ctx, void *p, int *refs)
{
	if (p)
	{
		int drop;
		(void)Memento_checkIntPointerOrNull(refs);
		fz_lock(ctx, FZ_LOCK_ALLOC);
		if (*refs > 0)
		{
			(void)Memento_dropIntRef(p);
			drop = --*refs == 0;
		}
		else
			drop = 0;
		fz_unlock(ctx, FZ_LOCK_ALLOC);
		return drop;
	}
	return 0;
}

static inline int
fz_drop_imp8(fz_context *ctx, void *p, int8_t *refs)
{
	if (p)
	{
		int drop;
		(void)Memento_checkBytePointerOrNull(refs);
		fz_lock(ctx, FZ_LOCK_ALLOC);
		if (*refs > 0)
		{
			(void)Memento_dropByteRef(p);
			drop = --*refs == 0;
		}
		else
			drop = 0;
		fz_unlock(ctx, FZ_LOCK_ALLOC);
		return drop;
	}
	return 0;
}

static inline int
fz_drop_imp16(fz_context *ctx, void *p, int16_t *refs)
{
	if (p)
	{
		int drop;
		(void)Memento_checkShortPointerOrNull(refs);
		fz_lock(ctx, FZ_LOCK_ALLOC);
		if (*refs > 0)
		{
			(void)Memento_dropShortRef(p);
			drop = --*refs == 0;
		}
		else
			drop = 0;
		fz_unlock(ctx, FZ_LOCK_ALLOC);
		return drop;
	}
	return 0;
}


/* Private sub contexts */

void fz_new_colorspace_context(fz_context *ctx);
fz_colorspace_context *fz_keep_colorspace_context(fz_context *ctx);
void fz_drop_colorspace_context(fz_context *ctx);

void fz_new_font_context(fz_context *ctx);

fz_font_context *fz_keep_font_context(fz_context *ctx);
void fz_drop_font_context(fz_context *ctx);

struct fz_tuning_context_s
{
	int refs;
	fz_tune_image_decode_fn *image_decode;
	void *image_decode_arg;
	fz_tune_image_scale_fn *image_scale;
	void *image_scale_arg;
};

void fz_default_image_decode(void *arg, int w, int h, int l2factor, fz_irect *subarea);
int fz_default_image_scale(void *arg, int dst_w, int dst_h, int src_w, int src_h);

fz_context *fz_clone_context_internal(fz_context *ctx);

void fz_init_aa_context(fz_context *ctx);

void fz_new_glyph_cache_context(fz_context *ctx);
fz_glyph_cache *fz_keep_glyph_cache(fz_context *ctx);
void fz_drop_glyph_cache_context(fz_context *ctx);

void fz_new_document_handler_context(fz_context *ctx);
void fz_drop_document_handler_context(fz_context *ctx);
fz_document_handler_context *fz_keep_document_handler_context(fz_context *ctx);

#endif
