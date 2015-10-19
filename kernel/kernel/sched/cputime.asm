
cputime.o:     file format elf32-littlearm


Disassembly of section .text:

00000000 <cputime_adjust>:
 * runtime accounting.
 */
static void cputime_adjust(struct task_cputime *curr,
			   struct cputime *prev,
			   cputime_t *ut, cputime_t *st)
{
   0:	e92d4ff8 	push	{r3, r4, r5, r6, r7, r8, r9, sl, fp, lr}
   4:	e1a05001 	mov	r5, r1
   8:	e1a07000 	mov	r7, r0
	 * cputime with a variable precision.
	 *
	 * Fix this by scaling these tick based values against the total
	 * runtime accounted by the CFS scheduler.
	 */
	rtime = nsecs_to_cputime(curr->sum_exec_runtime);
   c:	e1c000d8 	ldrd	r0, [r0, #8]
 * runtime accounting.
 */
static void cputime_adjust(struct task_cputime *curr,
			   struct cputime *prev,
			   cputime_t *ut, cputime_t *st)
{
  10:	e1a0a003 	mov	sl, r3
  14:	e1a0b002 	mov	fp, r2
	 * cputime with a variable precision.
	 *
	 * Fix this by scaling these tick based values against the total
	 * runtime accounted by the CFS scheduler.
	 */
	rtime = nsecs_to_cputime(curr->sum_exec_runtime);
  18:	ebfffffe 	bl	0 <nsecs_to_jiffies>
	/*
	 * Update userspace visible utime/stime values only if actual execution
	 * time is bigger than already exported. Note that can happen, that we
	 * provided bigger values due to scaling inaccuracy on big numbers.
	 */
	if (prev->stime + prev->utime >= rtime)
  1c:	e8950300 	ldm	r5, {r8, r9}
  20:	e0883009 	add	r3, r8, r9
  24:	e1500003 	cmp	r0, r3
	 * cputime with a variable precision.
	 *
	 * Fix this by scaling these tick based values against the total
	 * runtime accounted by the CFS scheduler.
	 */
	rtime = nsecs_to_cputime(curr->sum_exec_runtime);
  28:	e1a06000 	mov	r6, r0
	/*
	 * Update userspace visible utime/stime values only if actual execution
	 * time is bigger than already exported. Note that can happen, that we
	 * provided bigger values due to scaling inaccuracy on big numbers.
	 */
	if (prev->stime + prev->utime >= rtime)
  2c:	9a000009 	bls	58 <cputime_adjust+0x58>
		goto out;

	stime = curr->stime;
	utime = curr->utime;
  30:	e5974000 	ldr	r4, [r7]
	 * provided bigger values due to scaling inaccuracy on big numbers.
	 */
	if (prev->stime + prev->utime >= rtime)
		goto out;

	stime = curr->stime;
  34:	e5970004 	ldr	r0, [r7, #4]
	utime = curr->utime;

	if (utime == 0) {
  38:	e3540000 	cmp	r4, #0
  3c:	1a000009 	bne	68 <cputime_adjust+0x68>
	 * If the tick based count grows faster than the scheduler one,
	 * the result of the scaling may go backward.
	 * Let's enforce monotonicity.
	 */
	prev->stime = max(prev->stime, stime);
	prev->utime = max(prev->utime, utime);
  40:	e1540008 	cmp	r4, r8
  44:	21a08004 	movcs	r8, r4
	/*
	 * If the tick based count grows faster than the scheduler one,
	 * the result of the scaling may go backward.
	 * Let's enforce monotonicity.
	 */
	prev->stime = max(prev->stime, stime);
  48:	e1560009 	cmp	r6, r9
  4c:	25856004 	strcs	r6, [r5, #4]
  50:	35859004 	strcc	r9, [r5, #4]
	prev->utime = max(prev->utime, utime);
  54:	e5858000 	str	r8, [r5]

out:
	*ut = prev->utime;
  58:	e58b8000 	str	r8, [fp]
	*st = prev->stime;
  5c:	e5953004 	ldr	r3, [r5, #4]
  60:	e58a3000 	str	r3, [sl]
  64:	e8bd8ff8 	pop	{r3, r4, r5, r6, r7, r8, r9, sl, fp, pc}
	stime = curr->stime;
	utime = curr->utime;

	if (utime == 0) {
		stime = rtime;
	} else if (stime == 0) {
  68:	e3500000 	cmp	r0, #0
  6c:	01a04006 	moveq	r4, r6
  70:	01a06000 	moveq	r6, r0
  74:	0afffff1 	beq	40 <cputime_adjust+0x40>
		utime = rtime;
	} else {
		cputime_t total = stime + utime;
  78:	e0844000 	add	r4, r4, r0

	/*
	 * Make sure gcc understands that this is a 32x32->64 multiply,
	 * followed by a 64/32->64 divide.
	 */
	scaled = div_u64((u64) (u32) stime * (u64) (u32) rtime, (u32)total);
  7c:	e0810096 	umull	r0, r1, r6, r0
#define div64_ul(x, y)   div_u64((x), (y))

#ifndef div_u64_rem
static inline u64 div_u64_rem(u64 dividend, u32 divisor, u32 *remainder)
{
	*remainder = do_div(dividend, divisor);
  80:	ebfffffe 	bl	0 <__do_div64>
	} else {
		cputime_t total = stime + utime;

		stime = scale_stime((__force u64)stime,
				    (__force u64)rtime, (__force u64)total);
		utime = rtime - stime;
  84:	e0624006 	rsb	r4, r2, r6
	} else if (stime == 0) {
		utime = rtime;
	} else {
		cputime_t total = stime + utime;

		stime = scale_stime((__force u64)stime,
  88:	e1a06002 	mov	r6, r2
  8c:	eaffffeb 	b	40 <cputime_adjust+0x40>

00000090 <account_user_time>:
 * @cputime: the cpu time spent in user space since the last update
 * @cputime_scaled: cputime scaled by cpu frequency
 */
void account_user_time(struct task_struct *p, cputime_t cputime,
		       cputime_t cputime_scaled)
{
  90:	e92d47f0 	push	{r4, r5, r6, r7, r8, r9, sl, lr}
  94:	e1a04001 	mov	r4, r1
 * running CPU and update the utime field there.
 */
static inline void account_group_user_time(struct task_struct *tsk,
					   cputime_t cputime)
{
	struct thread_group_cputimer *cputimer = &tsk->signal->cputimer;
  98:	e59053e4 	ldr	r5, [r0, #996]	; 0x3e4
  9c:	e1a06000 	mov	r6, r0
	int index;

	/* Add user time to process. */
	p->utime += cputime;
	p->utimescaled += cputime_scaled;
  a0:	e59032c8 	ldr	r3, [r0, #712]	; 0x2c8
		       cputime_t cputime_scaled)
{
	int index;

	/* Add user time to process. */
	p->utime += cputime;
  a4:	e59012c0 	ldr	r1, [r0, #704]	; 0x2c0
	p->utimescaled += cputime_scaled;
  a8:	e0832002 	add	r2, r3, r2
  ac:	e58022c8 	str	r2, [r0, #712]	; 0x2c8
		       cputime_t cputime_scaled)
{
	int index;

	/* Add user time to process. */
	p->utime += cputime;
  b0:	e0811004 	add	r1, r1, r4
  b4:	e58012c0 	str	r1, [r0, #704]	; 0x2c0

	if (!cputimer->running)
  b8:	e59530d8 	ldr	r3, [r5, #216]	; 0xd8
  bc:	e3530000 	cmp	r3, #0
  c0:	0a000007 	beq	e4 <account_user_time+0x54>
		return;

	raw_spin_lock(&cputimer->lock);
  c4:	e28570dc 	add	r7, r5, #220	; 0xdc
  c8:	e1a00007 	mov	r0, r7
  cc:	ebfffffe 	bl	0 <_raw_spin_lock>
	cputimer->cputime.utime += cputime;
  d0:	e59530c8 	ldr	r3, [r5, #200]	; 0xc8
	raw_spin_unlock(&cputimer->lock);
  d4:	e1a00007 	mov	r0, r7

	if (!cputimer->running)
		return;

	raw_spin_lock(&cputimer->lock);
	cputimer->cputime.utime += cputime;
  d8:	e0833004 	add	r3, r3, r4
  dc:	e58530c8 	str	r3, [r5, #200]	; 0xc8
	raw_spin_unlock(&cputimer->lock);
  e0:	ebfffffe 	bl	0 <_raw_spin_unlock>
	p->utimescaled += cputime_scaled;
	account_group_user_time(p, cputime);

	index = (TASK_NICE(p) > 0) ? CPUTIME_NICE : CPUTIME_USER;
  e4:	e5969024 	ldr	r9, [r6, #36]	; 0x24
	 * Since all updates are sure to touch the root cgroup, we
	 * get ourselves ahead and touch it first. If the root cgroup
	 * is the only cgroup, then nothing else should be necessary.
	 *
	 */
	__get_cpu_var(kernel_cpustat).cpustat[index] += tmp;
  e8:	e3008000 	movw	r8, #0
  ec:	e3408000 	movt	r8, #0
  f0:	e3007000 	movw	r7, #0
	/* Add user time to process. */
	p->utime += cputime;
	p->utimescaled += cputime_scaled;
	account_group_user_time(p, cputime);

	index = (TASK_NICE(p) > 0) ? CPUTIME_NICE : CPUTIME_USER;
  f4:	e2499078 	sub	r9, r9, #120	; 0x78
	 * Since all updates are sure to touch the root cgroup, we
	 * get ourselves ahead and touch it first. If the root cgroup
	 * is the only cgroup, then nothing else should be necessary.
	 *
	 */
	__get_cpu_var(kernel_cpustat).cpustat[index] += tmp;
  f8:	e3407000 	movt	r7, #0
	/* Add user time to process. */
	p->utime += cputime;
	p->utimescaled += cputime_scaled;
	account_group_user_time(p, cputime);

	index = (TASK_NICE(p) > 0) ? CPUTIME_NICE : CPUTIME_USER;
  fc:	e3590000 	cmp	r9, #0

	/* Add user time to cpustat. */
	task_group_account_field(p, index, (__force u64) cputime);
 100:	e3a05000 	mov	r5, #0
	/* Add user time to process. */
	p->utime += cputime;
	p->utimescaled += cputime_scaled;
	account_group_user_time(p, cputime);

	index = (TASK_NICE(p) > 0) ? CPUTIME_NICE : CPUTIME_USER;
 104:	d3a09000 	movle	r9, #0
 108:	c3a09001 	movgt	r9, #1
	 * Since all updates are sure to touch the root cgroup, we
	 * get ourselves ahead and touch it first. If the root cgroup
	 * is the only cgroup, then nothing else should be necessary.
	 *
	 */
	__get_cpu_var(kernel_cpustat).cpustat[index] += tmp;
 10c:	ebfffffe 	bl	0 <debug_smp_processor_id>
 110:	e1a0c189 	lsl	ip, r9, #3

	cpuacct_account_field(p, index, tmp);
 114:	e1a02004 	mov	r2, r4
 118:	e1a01009 	mov	r1, r9
 11c:	e1a03005 	mov	r3, r5
	 * Since all updates are sure to touch the root cgroup, we
	 * get ourselves ahead and touch it first. If the root cgroup
	 * is the only cgroup, then nothing else should be necessary.
	 *
	 */
	__get_cpu_var(kernel_cpustat).cpustat[index] += tmp;
 120:	e7988100 	ldr	r8, [r8, r0, lsl #2]

	cpuacct_account_field(p, index, tmp);
 124:	e1a00006 	mov	r0, r6
	 * Since all updates are sure to touch the root cgroup, we
	 * get ourselves ahead and touch it first. If the root cgroup
	 * is the only cgroup, then nothing else should be necessary.
	 *
	 */
	__get_cpu_var(kernel_cpustat).cpustat[index] += tmp;
 128:	e0878008 	add	r8, r7, r8
 12c:	e18860dc 	ldrd	r6, [r8, ip]
 130:	e0944006 	adds	r4, r4, r6
 134:	e0a55007 	adc	r5, r5, r7
 138:	e18840fc 	strd	r4, [r8, ip]
	/* Add user time to cpustat. */
	task_group_account_field(p, index, (__force u64) cputime);

	/* Account for user time used */
	acct_account_cputime(p);
}
 13c:	e8bd47f0 	pop	{r4, r5, r6, r7, r8, r9, sl, lr}
	 * is the only cgroup, then nothing else should be necessary.
	 *
	 */
	__get_cpu_var(kernel_cpustat).cpustat[index] += tmp;

	cpuacct_account_field(p, index, tmp);
 140:	eafffffe 	b	0 <cpuacct_account_field>

00000144 <account_system_time>:
 * @cputime: the cpu time spent in kernel space since the last update
 * @cputime_scaled: cputime scaled by cpu frequency
 */
void account_system_time(struct task_struct *p, int hardirq_offset,
			 cputime_t cputime, cputime_t cputime_scaled)
{
 144:	e92d47f0 	push	{r4, r5, r6, r7, r8, r9, sl, lr}
 148:	e1a07000 	mov	r7, r0
	int index;

	if ((p->flags & PF_VCPU) && (irq_count() - hardirq_offset == 0)) {
 14c:	e590000c 	ldr	r0, [r0, #12]
 * @cputime: the cpu time spent in kernel space since the last update
 * @cputime_scaled: cputime scaled by cpu frequency
 */
void account_system_time(struct task_struct *p, int hardirq_offset,
			 cputime_t cputime, cputime_t cputime_scaled)
{
 150:	e24dd008 	sub	sp, sp, #8
 154:	e1a04002 	mov	r4, r2
	int index;

	if ((p->flags & PF_VCPU) && (irq_count() - hardirq_offset == 0)) {
 158:	e3100010 	tst	r0, #16
static inline struct thread_info *current_thread_info(void) __attribute_const__;

static inline struct thread_info *current_thread_info(void)
{
	register unsigned long sp asm ("sp");
	return (struct thread_info *)(sp & ~(THREAD_SIZE - 1));
 15c:	e1a0000d 	mov	r0, sp
 160:	e3c02d7f 	bic	r2, r0, #8128	; 0x1fc0
 164:	0a000005 	beq	180 <account_system_time+0x3c>
 168:	e3c2003f 	bic	r0, r2, #63	; 0x3f
 16c:	e5900004 	ldr	r0, [r0, #4]
 170:	e3c0033e 	bic	r0, r0, #-134217728	; 0xf8000000
 174:	e3c000ff 	bic	r0, r0, #255	; 0xff
 178:	e1500001 	cmp	r0, r1
 17c:	0a000032 	beq	24c <account_system_time+0x108>
 180:	e3c2203f 	bic	r2, r2, #63	; 0x3f
		account_guest_time(p, cputime, cputime_scaled);
		return;
	}

	if (hardirq_count() - hardirq_offset)
 184:	e3a00000 	mov	r0, #0
 188:	e34003ff 	movt	r0, #1023	; 0x3ff
 18c:	e5922004 	ldr	r2, [r2, #4]
 190:	e0020000 	and	r0, r2, r0
 194:	e1500001 	cmp	r0, r1
		index = CPUTIME_IRQ;
 198:	13a09004 	movne	r9, #4
	if ((p->flags & PF_VCPU) && (irq_count() - hardirq_offset == 0)) {
		account_guest_time(p, cputime, cputime_scaled);
		return;
	}

	if (hardirq_count() - hardirq_offset)
 19c:	0a000026 	beq	23c <account_system_time+0xf8>
 * running CPU and update the stime field there.
 */
static inline void account_group_system_time(struct task_struct *tsk,
					     cputime_t cputime)
{
	struct thread_group_cputimer *cputimer = &tsk->signal->cputimer;
 1a0:	e59753e4 	ldr	r5, [r7, #996]	; 0x3e4
static inline
void __account_system_time(struct task_struct *p, cputime_t cputime,
			cputime_t cputime_scaled, int index)
{
	/* Add system time to process. */
	p->stime += cputime;
 1a4:	e59712c4 	ldr	r1, [r7, #708]	; 0x2c4
	p->stimescaled += cputime_scaled;
 1a8:	e59722cc 	ldr	r2, [r7, #716]	; 0x2cc
static inline
void __account_system_time(struct task_struct *p, cputime_t cputime,
			cputime_t cputime_scaled, int index)
{
	/* Add system time to process. */
	p->stime += cputime;
 1ac:	e0811004 	add	r1, r1, r4
 1b0:	e58712c4 	str	r1, [r7, #708]	; 0x2c4
	p->stimescaled += cputime_scaled;
 1b4:	e0823003 	add	r3, r2, r3
 1b8:	e58732cc 	str	r3, [r7, #716]	; 0x2cc

	if (!cputimer->running)
 1bc:	e59530d8 	ldr	r3, [r5, #216]	; 0xd8
 1c0:	e3530000 	cmp	r3, #0
 1c4:	1a000013 	bne	218 <account_system_time+0xd4>
	 * Since all updates are sure to touch the root cgroup, we
	 * get ourselves ahead and touch it first. If the root cgroup
	 * is the only cgroup, then nothing else should be necessary.
	 *
	 */
	__get_cpu_var(kernel_cpustat).cpustat[index] += tmp;
 1c8:	ebfffffe 	bl	0 <debug_smp_processor_id>
 1cc:	e3006000 	movw	r6, #0
 1d0:	e3406000 	movt	r6, #0
 1d4:	e1a08189 	lsl	r8, r9, #3
 1d8:	e300c000 	movw	ip, #0
 1dc:	e340c000 	movt	ip, #0
	p->stime += cputime;
	p->stimescaled += cputime_scaled;
	account_group_system_time(p, cputime);

	/* Add system time to cpustat. */
	task_group_account_field(p, index, (__force u64) cputime);
 1e0:	e3a05000 	mov	r5, #0
	 * is the only cgroup, then nothing else should be necessary.
	 *
	 */
	__get_cpu_var(kernel_cpustat).cpustat[index] += tmp;

	cpuacct_account_field(p, index, tmp);
 1e4:	e1a02004 	mov	r2, r4
 1e8:	e1a03005 	mov	r3, r5
 1ec:	e1a01009 	mov	r1, r9
	 * Since all updates are sure to touch the root cgroup, we
	 * get ourselves ahead and touch it first. If the root cgroup
	 * is the only cgroup, then nothing else should be necessary.
	 *
	 */
	__get_cpu_var(kernel_cpustat).cpustat[index] += tmp;
 1f0:	e7966100 	ldr	r6, [r6, r0, lsl #2]

	cpuacct_account_field(p, index, tmp);
 1f4:	e1a00007 	mov	r0, r7
	 * Since all updates are sure to touch the root cgroup, we
	 * get ourselves ahead and touch it first. If the root cgroup
	 * is the only cgroup, then nothing else should be necessary.
	 *
	 */
	__get_cpu_var(kernel_cpustat).cpustat[index] += tmp;
 1f8:	e08cc006 	add	ip, ip, r6
 1fc:	e18c60d8 	ldrd	r6, [ip, r8]
 200:	e0944006 	adds	r4, r4, r6
 204:	e0a55007 	adc	r5, r5, r7
 208:	e18c40f8 	strd	r4, [ip, r8]
		index = CPUTIME_SOFTIRQ;
	else
		index = CPUTIME_SYSTEM;

	__account_system_time(p, cputime, cputime_scaled, index);
}
 20c:	e28dd008 	add	sp, sp, #8
 210:	e8bd47f0 	pop	{r4, r5, r6, r7, r8, r9, sl, lr}
	 * is the only cgroup, then nothing else should be necessary.
	 *
	 */
	__get_cpu_var(kernel_cpustat).cpustat[index] += tmp;

	cpuacct_account_field(p, index, tmp);
 214:	eafffffe 	b	0 <cpuacct_account_field>
		return;

	raw_spin_lock(&cputimer->lock);
 218:	e28560dc 	add	r6, r5, #220	; 0xdc
 21c:	e1a00006 	mov	r0, r6
 220:	ebfffffe 	bl	0 <_raw_spin_lock>
	cputimer->cputime.stime += cputime;
 224:	e59530cc 	ldr	r3, [r5, #204]	; 0xcc
	raw_spin_unlock(&cputimer->lock);
 228:	e1a00006 	mov	r0, r6

	if (!cputimer->running)
		return;

	raw_spin_lock(&cputimer->lock);
	cputimer->cputime.stime += cputime;
 22c:	e0833004 	add	r3, r3, r4
 230:	e58530cc 	str	r3, [r5, #204]	; 0xcc
	raw_spin_unlock(&cputimer->lock);
 234:	ebfffffe 	bl	0 <_raw_spin_unlock>
 238:	eaffffe2 	b	1c8 <account_system_time+0x84>
	}

	if (hardirq_count() - hardirq_offset)
		index = CPUTIME_IRQ;
	else if (in_serving_softirq())
		index = CPUTIME_SOFTIRQ;
 23c:	e3120c01 	tst	r2, #256	; 0x100
 240:	03a09002 	moveq	r9, #2
 244:	13a09003 	movne	r9, #3
 248:	eaffffd4 	b	1a0 <account_system_time+0x5c>
 * @cputime_scaled: cputime scaled by cpu frequency
 */
static void account_guest_time(struct task_struct *p, cputime_t cputime,
			       cputime_t cputime_scaled)
{
	u64 *cpustat = kcpustat_this_cpu->cpustat;
 24c:	e58d3004 	str	r3, [sp, #4]
 250:	ebfffffe 	bl	0 <debug_smp_processor_id>

	/* Add guest time to process. */
	p->utime += cputime;
 254:	e597c2c0 	ldr	ip, [r7, #704]	; 0x2c0
 * @cputime_scaled: cputime scaled by cpu frequency
 */
static void account_guest_time(struct task_struct *p, cputime_t cputime,
			       cputime_t cputime_scaled)
{
	u64 *cpustat = kcpustat_this_cpu->cpustat;
 258:	e3002000 	movw	r2, #0
 * running CPU and update the utime field there.
 */
static inline void account_group_user_time(struct task_struct *tsk,
					   cputime_t cputime)
{
	struct thread_group_cputimer *cputimer = &tsk->signal->cputimer;
 25c:	e59753e4 	ldr	r5, [r7, #996]	; 0x3e4
 260:	e3402000 	movt	r2, #0

	/* Add guest time to process. */
	p->utime += cputime;
 264:	e08cc004 	add	ip, ip, r4
 268:	e587c2c0 	str	ip, [r7, #704]	; 0x2c0
	p->utimescaled += cputime_scaled;
 26c:	e59712c8 	ldr	r1, [r7, #712]	; 0x2c8
 * @cputime_scaled: cputime scaled by cpu frequency
 */
static void account_guest_time(struct task_struct *p, cputime_t cputime,
			       cputime_t cputime_scaled)
{
	u64 *cpustat = kcpustat_this_cpu->cpustat;
 270:	e3008000 	movw	r8, #0

	/* Add guest time to process. */
	p->utime += cputime;
	p->utimescaled += cputime_scaled;
 274:	e59d3004 	ldr	r3, [sp, #4]
 * @cputime_scaled: cputime scaled by cpu frequency
 */
static void account_guest_time(struct task_struct *p, cputime_t cputime,
			       cputime_t cputime_scaled)
{
	u64 *cpustat = kcpustat_this_cpu->cpustat;
 278:	e3408000 	movt	r8, #0

	/* Add guest time to process. */
	p->utime += cputime;
	p->utimescaled += cputime_scaled;
 27c:	e0813003 	add	r3, r1, r3
 280:	e58732c8 	str	r3, [r7, #712]	; 0x2c8

	if (!cputimer->running)
 284:	e59530d8 	ldr	r3, [r5, #216]	; 0xd8
 288:	e3530000 	cmp	r3, #0
 * @cputime_scaled: cputime scaled by cpu frequency
 */
static void account_guest_time(struct task_struct *p, cputime_t cputime,
			       cputime_t cputime_scaled)
{
	u64 *cpustat = kcpustat_this_cpu->cpustat;
 28c:	e7929100 	ldr	r9, [r2, r0, lsl #2]
 290:	e0886009 	add	r6, r8, r9
 294:	1a00001b 	bne	308 <account_system_time+0x1c4>
	p->utimescaled += cputime_scaled;
	account_group_user_time(p, cputime);
	p->gtime += cputime;

	/* Add guest time to cpustat. */
	if (TASK_NICE(p) > 0) {
 298:	e5972024 	ldr	r2, [r7, #36]	; 0x24

	/* Add guest time to process. */
	p->utime += cputime;
	p->utimescaled += cputime_scaled;
	account_group_user_time(p, cputime);
	p->gtime += cputime;
 29c:	e59732d0 	ldr	r3, [r7, #720]	; 0x2d0

	/* Add guest time to cpustat. */
	if (TASK_NICE(p) > 0) {
 2a0:	e2422078 	sub	r2, r2, #120	; 0x78
 2a4:	e3520000 	cmp	r2, #0

	/* Add guest time to process. */
	p->utime += cputime;
	p->utimescaled += cputime_scaled;
	account_group_user_time(p, cputime);
	p->gtime += cputime;
 2a8:	e0833004 	add	r3, r3, r4
 2ac:	e58732d0 	str	r3, [r7, #720]	; 0x2d0

	/* Add guest time to cpustat. */
	if (TASK_NICE(p) > 0) {
 2b0:	da00000a 	ble	2e0 <account_system_time+0x19c>
		cpustat[CPUTIME_NICE] += (__force u64) cputime;
 2b4:	e1c600d8 	ldrd	r0, [r6, #8]
 2b8:	e3a05000 	mov	r5, #0
		cpustat[CPUTIME_GUEST_NICE] += (__force u64) cputime;
 2bc:	e1c624d8 	ldrd	r2, [r6, #72]	; 0x48
	account_group_user_time(p, cputime);
	p->gtime += cputime;

	/* Add guest time to cpustat. */
	if (TASK_NICE(p) > 0) {
		cpustat[CPUTIME_NICE] += (__force u64) cputime;
 2c0:	e0900004 	adds	r0, r0, r4
 2c4:	e0a11005 	adc	r1, r1, r5
		cpustat[CPUTIME_GUEST_NICE] += (__force u64) cputime;
 2c8:	e0922004 	adds	r2, r2, r4
 2cc:	e0a33005 	adc	r3, r3, r5
	account_group_user_time(p, cputime);
	p->gtime += cputime;

	/* Add guest time to cpustat. */
	if (TASK_NICE(p) > 0) {
		cpustat[CPUTIME_NICE] += (__force u64) cputime;
 2d0:	e1c600f8 	strd	r0, [r6, #8]
		cpustat[CPUTIME_GUEST_NICE] += (__force u64) cputime;
 2d4:	e1c624f8 	strd	r2, [r6, #72]	; 0x48
		index = CPUTIME_SOFTIRQ;
	else
		index = CPUTIME_SYSTEM;

	__account_system_time(p, cputime, cputime_scaled, index);
}
 2d8:	e28dd008 	add	sp, sp, #8
 2dc:	e8bd87f0 	pop	{r4, r5, r6, r7, r8, r9, sl, pc}
	/* Add guest time to cpustat. */
	if (TASK_NICE(p) > 0) {
		cpustat[CPUTIME_NICE] += (__force u64) cputime;
		cpustat[CPUTIME_GUEST_NICE] += (__force u64) cputime;
	} else {
		cpustat[CPUTIME_USER] += (__force u64) cputime;
 2e0:	e18800d9 	ldrd	r0, [r8, r9]
 2e4:	e3a05000 	mov	r5, #0
 2e8:	e0900004 	adds	r0, r0, r4
 2ec:	e0a11005 	adc	r1, r1, r5
 2f0:	e18800f9 	strd	r0, [r8, r9]
		cpustat[CPUTIME_GUEST] += (__force u64) cputime;
 2f4:	e1c624d0 	ldrd	r2, [r6, #64]	; 0x40
 2f8:	e0922004 	adds	r2, r2, r4
 2fc:	e0a33005 	adc	r3, r3, r5
 300:	e1c624f0 	strd	r2, [r6, #64]	; 0x40
 304:	eafffff3 	b	2d8 <account_system_time+0x194>
		return;

	raw_spin_lock(&cputimer->lock);
 308:	e285a0dc 	add	sl, r5, #220	; 0xdc
 30c:	e1a0000a 	mov	r0, sl
 310:	ebfffffe 	bl	0 <_raw_spin_lock>
	cputimer->cputime.utime += cputime;
 314:	e59530c8 	ldr	r3, [r5, #200]	; 0xc8
	raw_spin_unlock(&cputimer->lock);
 318:	e1a0000a 	mov	r0, sl

	if (!cputimer->running)
		return;

	raw_spin_lock(&cputimer->lock);
	cputimer->cputime.utime += cputime;
 31c:	e0833004 	add	r3, r3, r4
 320:	e58530c8 	str	r3, [r5, #200]	; 0xc8
	raw_spin_unlock(&cputimer->lock);
 324:	ebfffffe 	bl	0 <_raw_spin_unlock>
 328:	eaffffda 	b	298 <account_system_time+0x154>

0000032c <account_steal_time>:
/*
 * Account for involuntary wait time.
 * @cputime: the cpu time spent in involuntary wait
 */
void account_steal_time(cputime_t cputime)
{
 32c:	e92d4010 	push	{r4, lr}
 330:	e1a04000 	mov	r4, r0
	u64 *cpustat = kcpustat_this_cpu->cpustat;
 334:	ebfffffe 	bl	0 <debug_smp_processor_id>
 338:	e3002000 	movw	r2, #0
 33c:	e3402000 	movt	r2, #0
 340:	e3003000 	movw	r3, #0
 344:	e3403000 	movt	r3, #0
 348:	e7921100 	ldr	r1, [r2, r0, lsl #2]
 34c:	e0831001 	add	r1, r3, r1

	cpustat[CPUTIME_STEAL] += (__force u64) cputime;
 350:	e1c123d8 	ldrd	r2, [r1, #56]	; 0x38
 354:	e0922004 	adds	r2, r2, r4
 358:	e2a33000 	adc	r3, r3, #0
 35c:	e1c123f8 	strd	r2, [r1, #56]	; 0x38
 360:	e8bd8010 	pop	{r4, pc}

00000364 <account_idle_time>:
/*
 * Account for idle time.
 * @cputime: the cpu time spent in idle wait
 */
void account_idle_time(cputime_t cputime)
{
 364:	e92d40f8 	push	{r3, r4, r5, r6, r7, lr}
 368:	e1a04000 	mov	r4, r0
	u64 *cpustat = kcpustat_this_cpu->cpustat;
 36c:	ebfffffe 	bl	0 <debug_smp_processor_id>
 370:	e3006000 	movw	r6, #0
 374:	e3406000 	movt	r6, #0
 378:	e3003000 	movw	r3, #0
 37c:	e3403000 	movt	r3, #0
	struct rq *rq = this_rq();
 380:	e3005000 	movw	r5, #0
 384:	e3405000 	movt	r5, #0
 * Account for idle time.
 * @cputime: the cpu time spent in idle wait
 */
void account_idle_time(cputime_t cputime)
{
	u64 *cpustat = kcpustat_this_cpu->cpustat;
 388:	e7967100 	ldr	r7, [r6, r0, lsl #2]
 38c:	e0837007 	add	r7, r3, r7
	struct rq *rq = this_rq();
 390:	ebfffffe 	bl	0 <debug_smp_processor_id>
 394:	e7963100 	ldr	r3, [r6, r0, lsl #2]
 398:	e0855003 	add	r5, r5, r3

	if (atomic_read(&rq->nr_iowait) > 0)
 39c:	e59534a0 	ldr	r3, [r5, #1184]	; 0x4a0
 3a0:	e3530000 	cmp	r3, #0
 3a4:	da000004 	ble	3bc <account_idle_time+0x58>
		cpustat[CPUTIME_IOWAIT] += (__force u64) cputime;
 3a8:	e1c723d0 	ldrd	r2, [r7, #48]	; 0x30
 3ac:	e0922004 	adds	r2, r2, r4
 3b0:	e2a33000 	adc	r3, r3, #0
 3b4:	e1c723f0 	strd	r2, [r7, #48]	; 0x30
 3b8:	e8bd80f8 	pop	{r3, r4, r5, r6, r7, pc}
	else
		cpustat[CPUTIME_IDLE] += (__force u64) cputime;
 3bc:	e1c722d8 	ldrd	r2, [r7, #40]	; 0x28
 3c0:	e0922004 	adds	r2, r2, r4
 3c4:	e2a33000 	adc	r3, r3, #0
 3c8:	e1c722f8 	strd	r2, [r7, #40]	; 0x28
 3cc:	e8bd80f8 	pop	{r3, r4, r5, r6, r7, pc}

000003d0 <thread_group_cputime>:
/*
 * Accumulate raw cputime values of dead tasks (sig->[us]time) and live
 * tasks (sum on group iteration) belonging to @tsk's group.
 */
void thread_group_cputime(struct task_struct *tsk, struct task_cputime *times)
{
 3d0:	e92d41f0 	push	{r4, r5, r6, r7, r8, lr}
 3d4:	e1a07001 	mov	r7, r1
	struct signal_struct *sig = tsk->signal;
 3d8:	e59033e4 	ldr	r3, [r0, #996]	; 0x3e4
	cputime_t utime, stime;
	struct task_struct *t;

	times->utime = sig->utime;
	times->stime = sig->stime;
	times->sum_exec_runtime = sig->sum_sched_runtime;
 3dc:	e3a02e17 	mov	r2, #368	; 0x170
/*
 * Accumulate raw cputime values of dead tasks (sig->[us]time) and live
 * tasks (sum on group iteration) belonging to @tsk's group.
 */
void thread_group_cputime(struct task_struct *tsk, struct task_cputime *times)
{
 3e0:	e1a08000 	mov	r8, r0
 3e4:	e24dd008 	sub	sp, sp, #8
	cputime_t utime, stime;
	struct task_struct *t;

	times->utime = sig->utime;
	times->stime = sig->stime;
	times->sum_exec_runtime = sig->sum_sched_runtime;
 3e8:	e0832002 	add	r2, r3, r2
{
	struct signal_struct *sig = tsk->signal;
	cputime_t utime, stime;
	struct task_struct *t;

	times->utime = sig->utime;
 3ec:	e5931114 	ldr	r1, [r3, #276]	; 0x114
 3f0:	e5871000 	str	r1, [r7]
	times->stime = sig->stime;
 3f4:	e5931118 	ldr	r1, [r3, #280]	; 0x118
 3f8:	e5871004 	str	r1, [r7, #4]
	times->sum_exec_runtime = sig->sum_sched_runtime;
 3fc:	e1c220d0 	ldrd	r2, [r2]
 400:	e1c720f8 	strd	r2, [r7, #8]
 * block, but only when acquiring spinlocks that are subject to priority
 * inheritance.
 */
static inline void rcu_read_lock(void)
{
	__rcu_read_lock();
 404:	ebfffffe 	bl	0 <__rcu_read_lock>

	rcu_read_lock();
	/* make sure we can trust tsk->thread_group list */
	if (!likely(pid_alive(tsk)))
 408:	e5983290 	ldr	r3, [r8, #656]	; 0x290
 40c:	e3530000 	cmp	r3, #0
 410:	0a000014 	beq	468 <thread_group_cputime+0x98>
 414:	e1c740d8 	ldrd	r4, [r7, #8]
 418:	e1a06008 	mov	r6, r8
				cputime_t *utime, cputime_t *stime)
{
	if (utime)
		*utime = t->utime;
	if (stime)
		*stime = t->stime;
 41c:	e59612c4 	ldr	r1, [r6, #708]	; 0x2c4
	t = tsk;
	do {
		task_cputime(t, &utime, &stime);
		times->utime += utime;
		times->stime += stime;
		times->sum_exec_runtime += task_sched_runtime(t);
 420:	e1a00006 	mov	r0, r6
		goto out;

	t = tsk;
	do {
		task_cputime(t, &utime, &stime);
		times->utime += utime;
 424:	e59632c0 	ldr	r3, [r6, #704]	; 0x2c0
		times->stime += stime;
 428:	e597c004 	ldr	ip, [r7, #4]
		goto out;

	t = tsk;
	do {
		task_cputime(t, &utime, &stime);
		times->utime += utime;
 42c:	e5972000 	ldr	r2, [r7]
		times->stime += stime;
 430:	e08c1001 	add	r1, ip, r1
 434:	e5871004 	str	r1, [r7, #4]
		goto out;

	t = tsk;
	do {
		task_cputime(t, &utime, &stime);
		times->utime += utime;
 438:	e0823003 	add	r3, r2, r3
 43c:	e5873000 	str	r3, [r7]
		times->stime += stime;
		times->sum_exec_runtime += task_sched_runtime(t);
 440:	ebfffffe 	bl	0 <task_sched_runtime>
 444:	e0944000 	adds	r4, r4, r0
 448:	e0a55001 	adc	r5, r5, r1
 44c:	e1c740f8 	strd	r4, [r7, #8]
	return p1->tgid == p2->tgid;
}

static inline struct task_struct *next_thread(const struct task_struct *p)
{
	return list_entry_rcu(p->thread_group.next,
 450:	e59632ac 	ldr	r3, [r6, #684]	; 0x2ac
 454:	e58d3004 	str	r3, [sp, #4]
 458:	e59d6004 	ldr	r6, [sp, #4]
	} while_each_thread(tsk, t);
 45c:	e2466fab 	sub	r6, r6, #684	; 0x2ac
 460:	e1560008 	cmp	r6, r8
 464:	1affffec 	bne	41c <thread_group_cputime+0x4c>
out:
	rcu_read_unlock();
}
 468:	e28dd008 	add	sp, sp, #8
 46c:	e8bd41f0 	pop	{r4, r5, r6, r7, r8, lr}
{
	rcu_lockdep_assert(!rcu_is_cpu_idle(),
			   "rcu_read_unlock() used illegally while idle");
	rcu_lock_release(&rcu_lock_map);
	__release(RCU);
	__rcu_read_unlock();
 470:	eafffffe 	b	0 <__rcu_read_unlock>

00000474 <account_process_tick>:
 * Account a single tick of cpu time.
 * @p: the process that the cpu time gets accounted to
 * @user_tick: indicates if the tick is a user or a system tick
 */
void account_process_tick(struct task_struct *p, int user_tick)
{
 474:	e92d4070 	push	{r4, r5, r6, lr}
 478:	e1a05001 	mov	r5, r1
 47c:	e1a04000 	mov	r4, r0
	cputime_t one_jiffy_scaled = cputime_to_scaled(cputime_one_jiffy);
	struct rq *rq = this_rq();
 480:	ebfffffe 	bl	0 <debug_smp_processor_id>
 484:	e3002000 	movw	r2, #0
 488:	e3402000 	movt	r2, #0
	}

	if (steal_account_process_tick())
		return;

	if (user_tick)
 48c:	e3550000 	cmp	r5, #0
 * @user_tick: indicates if the tick is a user or a system tick
 */
void account_process_tick(struct task_struct *p, int user_tick)
{
	cputime_t one_jiffy_scaled = cputime_to_scaled(cputime_one_jiffy);
	struct rq *rq = this_rq();
 490:	e3003000 	movw	r3, #0
 494:	e3403000 	movt	r3, #0
 498:	e7922100 	ldr	r2, [r2, r0, lsl #2]
 49c:	e0833002 	add	r3, r3, r2
	}

	if (steal_account_process_tick())
		return;

	if (user_tick)
 4a0:	1a000013 	bne	4f4 <account_process_tick+0x80>
		account_user_time(p, cputime_one_jiffy, one_jiffy_scaled);
	else if ((p != rq->idle) || (irq_count() != HARDIRQ_OFFSET))
 4a4:	e593347c 	ldr	r3, [r3, #1148]	; 0x47c
 4a8:	e1530004 	cmp	r3, r4
 4ac:	0a000005 	beq	4c8 <account_process_tick+0x54>
		account_system_time(p, HARDIRQ_OFFSET, cputime_one_jiffy,
 4b0:	e3a02001 	mov	r2, #1
 4b4:	e1a00004 	mov	r0, r4
 4b8:	e1a03002 	mov	r3, r2
 4bc:	e3a01801 	mov	r1, #65536	; 0x10000
				    one_jiffy_scaled);
	else
		account_idle_time(cputime_one_jiffy);
}
 4c0:	e8bd4070 	pop	{r4, r5, r6, lr}
		return;

	if (user_tick)
		account_user_time(p, cputime_one_jiffy, one_jiffy_scaled);
	else if ((p != rq->idle) || (irq_count() != HARDIRQ_OFFSET))
		account_system_time(p, HARDIRQ_OFFSET, cputime_one_jiffy,
 4c4:	eafffffe 	b	144 <account_system_time>
 4c8:	e1a0200d 	mov	r2, sp
 4cc:	e3c23d7f 	bic	r3, r2, #8128	; 0x1fc0
 4d0:	e3c3303f 	bic	r3, r3, #63	; 0x3f
	if (steal_account_process_tick())
		return;

	if (user_tick)
		account_user_time(p, cputime_one_jiffy, one_jiffy_scaled);
	else if ((p != rq->idle) || (irq_count() != HARDIRQ_OFFSET))
 4d4:	e5933004 	ldr	r3, [r3, #4]
 4d8:	e3c3333e 	bic	r3, r3, #-134217728	; 0xf8000000
 4dc:	e3c330ff 	bic	r3, r3, #255	; 0xff
 4e0:	e3530801 	cmp	r3, #65536	; 0x10000
 4e4:	1afffff1 	bne	4b0 <account_process_tick+0x3c>
		account_system_time(p, HARDIRQ_OFFSET, cputime_one_jiffy,
				    one_jiffy_scaled);
	else
		account_idle_time(cputime_one_jiffy);
 4e8:	e3a00001 	mov	r0, #1
}
 4ec:	e8bd4070 	pop	{r4, r5, r6, lr}
		account_user_time(p, cputime_one_jiffy, one_jiffy_scaled);
	else if ((p != rq->idle) || (irq_count() != HARDIRQ_OFFSET))
		account_system_time(p, HARDIRQ_OFFSET, cputime_one_jiffy,
				    one_jiffy_scaled);
	else
		account_idle_time(cputime_one_jiffy);
 4f0:	eafffffe 	b	364 <account_idle_time>

	if (steal_account_process_tick())
		return;

	if (user_tick)
		account_user_time(p, cputime_one_jiffy, one_jiffy_scaled);
 4f4:	e3a01001 	mov	r1, #1
 4f8:	e1a00004 	mov	r0, r4
 4fc:	e1a02001 	mov	r2, r1
	else if ((p != rq->idle) || (irq_count() != HARDIRQ_OFFSET))
		account_system_time(p, HARDIRQ_OFFSET, cputime_one_jiffy,
				    one_jiffy_scaled);
	else
		account_idle_time(cputime_one_jiffy);
}
 500:	e8bd4070 	pop	{r4, r5, r6, lr}

	if (steal_account_process_tick())
		return;

	if (user_tick)
		account_user_time(p, cputime_one_jiffy, one_jiffy_scaled);
 504:	eafffffe 	b	90 <account_user_time>

00000508 <account_steal_ticks>:
 * @p: the process from which the cpu time has been stolen
 * @ticks: number of stolen ticks
 */
void account_steal_ticks(unsigned long ticks)
{
	account_steal_time(jiffies_to_cputime(ticks));
 508:	eafffffe 	b	32c <account_steal_time>

0000050c <account_idle_ticks>:
	if (sched_clock_irqtime) {
		irqtime_account_idle_ticks(ticks);
		return;
	}

	account_idle_time(jiffies_to_cputime(ticks));
 50c:	eafffffe 	b	364 <account_idle_time>

00000510 <task_cputime_adjusted>:
	*ut = prev->utime;
	*st = prev->stime;
}

void task_cputime_adjusted(struct task_struct *p, cputime_t *ut, cputime_t *st)
{
 510:	e92d4030 	push	{r4, r5, lr}
 514:	e24dd014 	sub	sp, sp, #20
	struct task_cputime cputime = {
 518:	e1c046d0 	ldrd	r4, [r0, #96]	; 0x60
	*ut = prev->utime;
	*st = prev->stime;
}

void task_cputime_adjusted(struct task_struct *p, cputime_t *ut, cputime_t *st)
{
 51c:	e1a03002 	mov	r3, r2
#else
static inline void task_cputime(struct task_struct *t,
				cputime_t *utime, cputime_t *stime)
{
	if (utime)
		*utime = t->utime;
 520:	e590e2c0 	ldr	lr, [r0, #704]	; 0x2c0
	struct task_cputime cputime = {
		.sum_exec_runtime = p->se.sum_exec_runtime,
	};

	task_cputime(p, &cputime.utime, &cputime.stime);
	cputime_adjust(&cputime, &p->prev_cputime, ut, st);
 524:	e1a02001 	mov	r2, r1
	if (stime)
		*stime = t->stime;
 528:	e590c2c4 	ldr	ip, [r0, #708]	; 0x2c4
 52c:	e2801fb5 	add	r1, r0, #724	; 0x2d4
 530:	e1a0000d 	mov	r0, sp
	*st = prev->stime;
}

void task_cputime_adjusted(struct task_struct *p, cputime_t *ut, cputime_t *st)
{
	struct task_cputime cputime = {
 534:	e1cd40f8 	strd	r4, [sp, #8]
#else
static inline void task_cputime(struct task_struct *t,
				cputime_t *utime, cputime_t *stime)
{
	if (utime)
		*utime = t->utime;
 538:	e58de000 	str	lr, [sp]
	if (stime)
		*stime = t->stime;
 53c:	e58dc004 	str	ip, [sp, #4]
		.sum_exec_runtime = p->se.sum_exec_runtime,
	};

	task_cputime(p, &cputime.utime, &cputime.stime);
	cputime_adjust(&cputime, &p->prev_cputime, ut, st);
 540:	ebfffeae 	bl	0 <cputime_adjust>
}
 544:	e28dd014 	add	sp, sp, #20
 548:	e8bd8030 	pop	{r4, r5, pc}

0000054c <thread_group_cputime_adjusted>:

/*
 * Must be called with siglock held.
 */
void thread_group_cputime_adjusted(struct task_struct *p, cputime_t *ut, cputime_t *st)
{
 54c:	e92d4070 	push	{r4, r5, r6, lr}
 550:	e24dd010 	sub	sp, sp, #16
 554:	e1a06000 	mov	r6, r0
 558:	e1a05001 	mov	r5, r1
	struct task_cputime cputime;

	thread_group_cputime(p, &cputime);
 55c:	e1a0100d 	mov	r1, sp

/*
 * Must be called with siglock held.
 */
void thread_group_cputime_adjusted(struct task_struct *p, cputime_t *ut, cputime_t *st)
{
 560:	e1a04002 	mov	r4, r2
	struct task_cputime cputime;

	thread_group_cputime(p, &cputime);
 564:	ebfffffe 	bl	3d0 <thread_group_cputime>
	cputime_adjust(&cputime, &p->signal->prev_cputime, ut, st);
 568:	e59613e4 	ldr	r1, [r6, #996]	; 0x3e4
 56c:	e1a0000d 	mov	r0, sp
 570:	e1a02005 	mov	r2, r5
 574:	e2811f4b 	add	r1, r1, #300	; 0x12c
 578:	e1a03004 	mov	r3, r4
 57c:	ebfffe9f 	bl	0 <cputime_adjust>
}
 580:	e28dd010 	add	sp, sp, #16
 584:	e8bd8070 	pop	{r4, r5, r6, pc}
