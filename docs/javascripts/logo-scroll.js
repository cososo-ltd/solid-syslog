/* Shrink the header logo on scroll to match cososo.co.uk (SiteOrigin Corp):
   scale 1.0 at the top -> 0.755 after 48px of scroll. */
(function () {
  var THRESHOLD = 48, MIN_SCALE = 0.755, ticking = false;
  function apply() {
    var y = window.pageYOffset || document.documentElement.scrollTop || 0;
    var t = Math.max(0, THRESHOLD - y) / THRESHOLD;   // 1 at top -> 0 past 48px
    var scale = MIN_SCALE + t * (1 - MIN_SCALE);       // 1.0 -> 0.755
    document.documentElement.style.setProperty('--logo-scale', scale.toFixed(4));
    ticking = false;
  }
  function onScroll() { if (!ticking) { requestAnimationFrame(apply); ticking = true; } }
  window.addEventListener('scroll', onScroll, { passive: true });
  if (window.document$) { window.document$.subscribe(apply); } // re-run after Material instant-nav
  apply();
})();
