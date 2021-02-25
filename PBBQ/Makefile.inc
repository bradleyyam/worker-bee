# -*-Makefile-*-

OBJDIRS	+= $(KERN_OBJDIR)/thread/PBBQ

KERN_SRCFILES += $(KERN_DIR)/thread/PBBQ/PBBQ.c

$(KERN_OBJDIR)/thread/PBBQ/%.o: $(KERN_DIR)/thread/PBBQ/%.c
	@echo + $(COMP_NAME)[KERN/thread/PBBQ] $<
	@mkdir -p $(@D)
	$(V)$(CCOMP) $(CCOMP_KERN_CFLAGS) -c -o $@ $<

$(KERN_OBJDIR)/thread/PKCtxIntro/%.o: $(KERN_DIR)/thread/PBBQ/%.S
	@echo + as[KERN/thread/PBBQ] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(KERN_CFLAGS) -c -o $@ $<
