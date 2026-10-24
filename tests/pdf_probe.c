#include <mupdf/fitz.h>
#include <mupdf/pdf.h>
#include <stdio.h>
#include <string.h>

static void show_metadata(fz_context *ctx, fz_document *doc, const char *key)
{
	char buf[512];
	int n = fz_lookup_metadata(ctx, doc, key, buf, sizeof(buf));
	if (n >= 0 && buf[0])
		printf("meta.%s: %s\n", key, buf);
}

int main(int argc, char **argv)
{
	fz_context *ctx;
	fz_document *doc = NULL;
	pdf_document *pdf = NULL;
	const char *path;
	int i;

	setvbuf(stdout, NULL, _IONBF, 0);
	if (argc < 2) {
		fprintf(stderr, "uso: pdf_probe <arquivo.pdf>\n");
		return 2;
	}
	path = argv[1];

	ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
	if (!ctx) {
		fprintf(stderr, "sem contexto\n");
		return 1;
	}
	fz_register_document_handlers(ctx);

	fz_try(ctx) {
		doc = fz_open_document(ctx, path);
	}
	fz_catch(ctx) {
		fprintf(stderr, "erro ao abrir: %s\n", fz_caught_message(ctx));
		fz_drop_context(ctx);
		return 1;
	}

	printf("arquivo: %s\n", path);
	printf("needs_password: %d\n", fz_needs_password(ctx, doc));

	show_metadata(ctx, doc, FZ_META_ENCRYPTION);
	show_metadata(ctx, doc, FZ_META_INFO_TITLE);
	show_metadata(ctx, doc, FZ_META_INFO_AUTHOR);
	show_metadata(ctx, doc, FZ_META_INFO_PRODUCER);
	show_metadata(ctx, doc, FZ_META_INFO_CREATOR);
	show_metadata(ctx, doc, FZ_META_FORMAT);

	if (fz_needs_password(ctx, doc)) {
		printf("conclusao: exige senha de abertura; analise interrompida (sem tentativa de quebra).\n");
		fz_drop_document(ctx, doc);
		fz_drop_context(ctx);
		return 0;
	}

	{
		static const struct { const char *name; fz_permission perm; } perms[] = {
			{ "imprimir", FZ_PERMISSION_PRINT },
			{ "copiar", FZ_PERMISSION_COPY },
			{ "editar", FZ_PERMISSION_EDIT },
			{ "anotar", FZ_PERMISSION_ANNOTATE },
			{ "formularios", FZ_PERMISSION_FORM },
			{ "acessibilidade", FZ_PERMISSION_ACCESSIBILITY },
			{ "montar", FZ_PERMISSION_ASSEMBLE },
			{ "imprimir_alta", FZ_PERMISSION_PRINT_HQ },
		};
		for (i = 0; i < (int)(sizeof(perms) / sizeof(perms[0])); i++)
			printf("permissao.%s: %d\n", perms[i].name,
				fz_has_permission(ctx, doc, perms[i].perm));
	}

	printf("paginas: %d\n", fz_count_pages(ctx, doc));

	pdf = pdf_specifics(ctx, doc);
	if (!pdf) {
		printf("nao e um PDF nativo do MuPDF\n");
		fz_drop_document(ctx, doc);
		fz_drop_context(ctx);
		return 0;
	}

	fz_try(ctx) {
		pdf_obj *trailer = pdf_trailer(ctx, pdf);
		pdf_obj *enc = pdf_dict_getp(ctx, trailer, "Encrypt");
		pdf_obj *root = pdf_dict_getp(ctx, trailer, "Root");
		pdf_obj *info = pdf_dict_getp(ctx, trailer, "Info");
		pdf_obj *obj;

		printf("trailer.Encrypt: %s\n", enc ? "presente" : "ausente");
		if (enc) {
			const char *filter = pdf_to_name(ctx, pdf_dict_getp(ctx, enc, "Filter"));
			int v = pdf_to_int(ctx, pdf_dict_getp(ctx, enc, "V"));
			int r = pdf_to_int(ctx, pdf_dict_getp(ctx, enc, "R"));
			int length = pdf_to_int(ctx, pdf_dict_getp(ctx, enc, "Length"));
			int p = pdf_to_int(ctx, pdf_dict_getp(ctx, enc, "P"));
			int meta = pdf_to_bool(ctx, pdf_dict_getp(ctx, enc, "EncryptMetadata"));
			int stmf = pdf_to_name(ctx, pdf_dict_getp(ctx, enc, "StmF")) ? 1 : 0;
			int strf = pdf_to_name(ctx, pdf_dict_getp(ctx, enc, "StrF")) ? 1 : 0;
			printf("encrypt.Filter: %s\n", filter ? filter : "(nulo)");
			printf("encrypt.V: %d\n", v);
			printf("encrypt.R: %d\n", r);
			printf("encrypt.Length: %d\n", length);
			printf("encrypt.P: %d (0x%08X)\n", p, (unsigned)p);
			printf("encrypt.EncryptMetadata: %d\n", meta);
			printf("encrypt.filtros_por_objeto: StmF=%d StrF=%d\n", stmf, strf);
			printf("encrypt.O: %s\n", pdf_dict_getp(ctx, enc, "O") ? "presente" : "ausente");
			printf("encrypt.U: %s\n", pdf_dict_getp(ctx, enc, "U") ? "presente" : "ausente");
			printf("encrypt.OE: %s\n", pdf_dict_getp(ctx, enc, "OE") ? "presente" : "ausente");
			printf("encrypt.UE: %s\n", pdf_dict_getp(ctx, enc, "UE") ? "presente" : "ausente");
			printf("encrypt.CF: %s\n", pdf_dict_getp(ctx, enc, "CF") ? "presente" : "ausente");
		}

		obj = pdf_dict_getp(ctx, root, "Perms/DocMDP");
		printf("root.Perms.DocMDP: %s\n", obj ? "presente" : "ausente");
		if (obj) {
			const char *sub = pdf_to_name(ctx, pdf_dict_getp(ctx, obj, "SubFilter"));
			printf("docmdp.SubFilter: %s\n", sub ? sub : "(nulo)");
			printf("docmdp.ByteRange: %s\n", pdf_dict_getp(ctx, obj, "ByteRange") ? "presente" : "ausente");
			printf("docmdp.P: %d\n", pdf_to_int(ctx, pdf_dict_getp(ctx, obj, "P")));
		}
		obj = pdf_dict_getp(ctx, root, "Perms/UR3");
		printf("root.Perms.UR3: %s\n", obj ? "presente" : "ausente");

		obj = pdf_dict_getp(ctx, root, "AcroForm");
		printf("root.AcroForm: %s\n", obj ? "presente" : "ausente");
		if (obj) {
			pdf_obj *xfa = pdf_dict_getp(ctx, obj, "XFA");
			pdf_obj *fields = pdf_dict_getp(ctx, obj, "Fields");
			int n = fields ? pdf_array_len(ctx, fields) : 0;
			int sigs = 0;
			printf("acroform.XFA: %s\n", xfa ? (pdf_is_array(ctx, xfa) ? "presente (array)" : "presente") : "ausente");
			printf("acroform.Fields: %d\n", n);
			for (i = 0; i < n; i++) {
				pdf_obj *field = pdf_array_get(ctx, fields, i);
				const char *ft = pdf_to_name(ctx, pdf_dict_getp(ctx, field, "FT"));
				pdf_obj *tobj = pdf_dict_getp(ctx, field, "T");
				const char *t = tobj ? pdf_to_text_string(ctx, tobj) : NULL;
				pdf_obj *sig = pdf_dict_getp(ctx, field, "V");
				if (ft && !strcmp(ft, "Sig")) {
					sigs++;
					printf("assinatura[%d]: campo=%s\n", i, t ? t : "(sem nome)");
					if (sig) {
						const char *sf = pdf_to_name(ctx, pdf_dict_getp(ctx, sig, "SubFilter"));
						pdf_obj *br = pdf_dict_getp(ctx, sig, "ByteRange");
						pdf_obj *ro = pdf_dict_getp(ctx, sig, "Reason");
						pdf_obj *so = pdf_dict_getp(ctx, sig, "Name");
						pdf_obj *mo = pdf_dict_getp(ctx, sig, "M");
						const char *reason = ro ? pdf_to_text_string(ctx, ro) : NULL;
						const char *signer = so ? pdf_to_text_string(ctx, so) : NULL;
						const char *m = mo ? pdf_to_text_string(ctx, mo) : NULL;
						printf("  assinatura.SubFilter: %s\n", sf ? sf : "(nulo)");
						printf("  assinatura.ByteRange: %s\n", br ? "presente" : "ausente");
						printf("  assinatura.assina: %s\n", signer ? signer : "(nao informado)");
						printf("  assinatura.motivo: %s\n", reason ? reason : "(nao informado)");
						printf("  assinatura.data: %s\n", m ? m : "(nao informada)");
					} else {
						printf("  campo de assinatura vazio (nao assinado)\n");
					}
				}
			}
			printf("assinaturas: %d\n", sigs);
		}

		obj = pdf_dict_getp(ctx, root, "Names/JavaScript");
		printf("root.Names.JavaScript: %s\n", obj ? "presente" : "ausente");
		obj = pdf_dict_getp(ctx, root, "Names/EmbeddedFiles");
		printf("root.Names.EmbeddedFiles: %s\n", obj ? "presente" : "ausente");
		obj = pdf_dict_getp(ctx, root, "OpenAction");
		printf("root.OpenAction: %s\n", obj ? "presente" : "ausente");
		obj = pdf_dict_getp(ctx, root, "AA");
		printf("root.AA: %s\n", obj ? "presente" : "ausente");
		obj = pdf_dict_getp(ctx, root, "Collection");
		printf("root.Collection: %s\n", obj ? "presente" : "ausente");
		obj = pdf_dict_getp(ctx, root, "MarkInfo/Marked");
		printf("root.MarkInfo.Marked: %d\n", obj ? pdf_to_bool(ctx, obj) : 0);
		obj = pdf_dict_getp(ctx, root, "Lang");
		printf("root.Lang: %s\n", obj ? pdf_to_text_string(ctx, obj) : "(ausente)");
		obj = pdf_dict_getp(ctx, info, "Producer");
		printf("info.Producer: %s\n", obj ? pdf_to_text_string(ctx, obj) : "(ausente)");
		obj = pdf_dict_getp(ctx, info, "Creator");
		printf("info.Creator: %s\n", obj ? pdf_to_text_string(ctx, obj) : "(ausente)");
	}
	fz_catch(ctx) {
		printf("erro estrutural: %s\n", fz_caught_message(ctx));
	}

	/* Texto por pagina: confirma se a extracao direta funciona em cada uma. */
	{
		fz_page *page = NULL;
		fz_stext_page *text = NULL;
		fz_buffer *buf = NULL;
		fz_output *out = NULL;
		size_t len = 0;
		unsigned char *data = NULL;
		int chars = 0;
		int pno = 0;
		int npages = fz_count_pages(ctx, doc);
		int pages_with_text = 0;

		for (pno = 0; pno < npages; pno++) {
			chars = 0;
			len = 0;
			data = NULL;
			fz_var(page);
			fz_var(text);
			fz_var(buf);
			fz_var(out);
			fz_try(ctx) {
				page = fz_load_page(ctx, doc, pno);
				text = fz_new_stext_page_from_page(ctx, page, NULL);
				buf = fz_new_buffer(ctx, 4096);
				out = fz_new_output_with_buffer(ctx, buf);
				fz_print_stext_page_as_text(ctx, out, text);
				fz_close_output(ctx, out);
				out = NULL;
				len = fz_buffer_storage(ctx, buf, &data);
				for (size_t k = 0; k < len; k++) {
					if (data[k] != '\r' && data[k] != '\n' && data[k] != ' ')
						chars++;
				}
				if (pno == 0) {
					printf("pagina1.inicio: ");
					for (size_t k = 0; k < len && k < 160; k++) {
						unsigned char c = data[k];
						if (c == '\r' || c == '\n')
							c = ' ';
						if (c >= 32 && c < 127)
							putchar(c);
					}
					printf("\n");
				}
			}
			fz_always(ctx) {
				fz_drop_output(ctx, out);
				fz_drop_buffer(ctx, buf);
				fz_drop_stext_page(ctx, text);
				fz_drop_page(ctx, page);
				page = NULL;
				text = NULL;
				buf = NULL;
				out = NULL;
			}
			fz_catch(ctx) {
				printf("pagina%d.texto: erro (%s)\n", pno + 1, fz_caught_message(ctx));
				continue;
			}
			printf("pagina%d.caracteres_uteis: %d\n", pno + 1, chars);
			if (chars > 0)
				pages_with_text++;
		}
		printf("paginas_com_texto: %d de %d\n", pages_with_text, npages);
	}

	fz_drop_document(ctx, doc);
	fz_drop_context(ctx);
	return 0;
}
