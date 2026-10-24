#include <mupdf/fitz.h>
#include <stdio.h>
#include <string.h>

static void print_text(fz_context *ctx, fz_stext_page *text)
{
	fz_buffer *buf = NULL;
	fz_output *out = NULL;
	size_t len = 0;
	unsigned char *data;

	fz_var(buf);
	fz_var(out);

	fz_try(ctx) {
		buf = fz_new_buffer(ctx, 1024);
		out = fz_new_output_with_buffer(ctx, buf);
		fz_print_stext_page_as_text(ctx, out, text);
		fz_close_output(ctx, out);
		len = fz_buffer_storage(ctx, buf, &data);
		fwrite(data, 1, len, stdout);
	}
	fz_always(ctx) {
		fz_drop_output(ctx, out);
	}
	fz_catch(ctx) {
		fprintf(stderr, "erro ao extrair texto: %s\n", fz_caught_message(ctx));
	}
	fz_drop_buffer(ctx, buf);
}

int main(int argc, char **argv)
{
	const char *path = argc > 1 ? argv[1] : "samples/exemplo.pdf";
	const char *password = argc > 2 ? argv[2] : NULL;
	const char *outpng = argc > 3 ? argv[3] : "smoke_page.png";

	setvbuf(stdout, NULL, _IONBF, 0);

	fz_context *ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
	if (!ctx) {
		fprintf(stderr, "falha ao criar contexto MuPDF\n");
		return 1;
	}
	fz_register_document_handlers(ctx);

	fz_document *doc = NULL;
	fz_page *page = NULL;
	fz_pixmap *pix = NULL;
	fz_stext_page *text = NULL;
	int rc = 0;

	fz_try(ctx) {
		doc = fz_open_document(ctx, path);
		if (fz_needs_password(ctx, doc)) {
			int ok = password && fz_authenticate_password(ctx, doc, password);
			printf("protegido: sim\nautenticacao: %s\n", ok ? "ok" : "falhou");
			if (!ok)
				fz_throw(ctx, FZ_ERROR_FORMAT, "senha necessaria");
		} else {
			printf("protegido: nao\n");
		}
		int n = fz_count_pages(ctx, doc);
		printf("paginas: %d\n", n);
		if (n < 1)
			fz_throw(ctx, FZ_ERROR_FORMAT, "documento sem paginas");
		page = fz_load_page(ctx, doc, 0);
		fz_rect b = fz_bound_page(ctx, page);
		printf("pagina0: %.1f x %.1f pt\n", b.x1 - b.x0, b.y1 - b.y0);

		text = fz_new_stext_page_from_page(ctx, page, NULL);
		printf("--- texto ---\n");
		print_text(ctx, text);
		printf("--- fim ---\n");
		fz_drop_stext_page(ctx, text);
		text = NULL;

		pix = fz_new_pixmap_from_page(ctx, page, fz_scale(2, 2), fz_device_rgb(ctx), 0);
		fz_save_pixmap_as_png(ctx, pix, outpng);
		printf("render: %s %dx%d\n", outpng,
			fz_pixmap_width(ctx, pix), fz_pixmap_height(ctx, pix));
	}
	fz_always(ctx) {
		fz_drop_stext_page(ctx, text);
		fz_drop_pixmap(ctx, pix);
		fz_drop_page(ctx, page);
		fz_drop_document(ctx, doc);
	}
	fz_catch(ctx) {
		fprintf(stderr, "erro: %s\n", fz_caught_message(ctx));
		rc = 1;
	}
	fz_drop_context(ctx);
	return rc;
}
