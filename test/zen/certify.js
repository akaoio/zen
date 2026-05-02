// Tests for zen.certify() — ZEN port of zen.certify
import assert from 'assert';
import ZEN from '../../zen.js';

// Compact cert is a signed string: <86 base62><v>:<payload> or <86 base62><v>/<curve>:<payload>
function isCompactSig(s) {
  return (
    typeof s === 'string' &&
    s.length >= 88 &&
    /^[0-9A-Za-z]{86}[01]/.test(s) &&
    (s[87] === ':' || s[87] === '/')
  );
}

describe('zen.certify()', function () {
  this.timeout(20 * 1000);

  var alice, bob, carol;

  before(async function () {
    alice = await ZEN.pair();
    bob = await ZEN.pair();
    carol = await ZEN.pair();
  });

  describe('basic write policy', function () {
    it('certifies a single pub with a string policy', async function () {
      const out = await ZEN.certify(bob.pub, 'inbox', alice);
      assert.ok(isCompactSig(out), 'output should be a compact signed string');
    });
    it('cert payload contains correct c and w fields', async function () {
      const out = await ZEN.certify(bob.pub, 'inbox', alice);
      const data = await ZEN.verify(out, alice.pub);
      assert.strictEqual(data.c, bob.pub, 'c should be the certificant pub');
      assert.strictEqual(data.w, 'inbox', 'w should be the write policy');
    });
    it('certificate verifies with authority pub', async function () {
      const out = await ZEN.certify(bob.pub, 'inbox', alice);
      const verified = await ZEN.verify(out, alice.pub);
      assert.ok(verified !== undefined);
      assert.strictEqual(verified.c, bob.pub);
      assert.strictEqual(verified.w, 'inbox');
    });
  });

  describe('wildcard certificants', function () {
    it('string "*" is rejected', async function () {
      const out = await ZEN.certify('*', 'messages', alice);
      assert.strictEqual(out, undefined);
    });
    it('array containing "*" is rejected', async function () {
      const out = await ZEN.certify([bob.pub, '*'], 'messages', alice);
      assert.strictEqual(out, undefined);
    });
  });

  describe('array of certificants', function () {
    it('array of pub strings', async function () {
      const out = await ZEN.certify([bob.pub, carol.pub], 'inbox', alice);
      const data = await ZEN.verify(out, alice.pub);
      assert.ok(Array.isArray(data.c));
      assert.ok(data.c.includes(bob.pub));
      assert.ok(data.c.includes(carol.pub));
    });
    it('single-element array unwraps to string', async function () {
      const out = await ZEN.certify([bob.pub], 'inbox', alice);
      const data = await ZEN.verify(out, alice.pub);
      assert.strictEqual(data.c, bob.pub);
    });
    it('array of objects with .pub', async function () {
      const out = await ZEN.certify([bob, carol], 'inbox', alice);
      const data = await ZEN.verify(out, alice.pub);
      assert.ok(Array.isArray(data.c));
      assert.ok(data.c.includes(bob.pub));
    });
  });

  describe('object with .pub as certificant', function () {
    it('single object with .pub', async function () {
      const out = await ZEN.certify(bob, 'inbox', alice);
      const data = await ZEN.verify(out, alice.pub);
      assert.strictEqual(data.c, bob.pub);
    });
  });

  describe('policy forms', function () {
    it('RAD/LEX object as write policy', async function () {
      const pol = { '#': 'inbox', '.': '*' };
      const out = await ZEN.certify(bob.pub, pol, alice);
      const data = await ZEN.verify(out, alice.pub);
      assert.deepStrictEqual(data.w, pol);
    });
    it('array of policies as write policy', async function () {
      const pol = ['inbox', 'outbox'];
      const out = await ZEN.certify(bob.pub, pol, alice);
      const data = await ZEN.verify(out, alice.pub);
      assert.deepStrictEqual(data.w, pol);
    });
    it('policy.read and policy.write both set', async function () {
      const pol = { read: 'pub', write: 'inbox' };
      const out = await ZEN.certify(bob.pub, pol, alice);
      const data = await ZEN.verify(out, alice.pub);
      assert.strictEqual(data.r, 'pub');
      assert.strictEqual(data.w, 'inbox');
    });
    it('policy.read only', async function () {
      const pol = { read: 'pub' };
      const out = await ZEN.certify(bob.pub, pol, alice);
      const data = await ZEN.verify(out, alice.pub);
      assert.strictEqual(data.r, 'pub');
    });
  });

  describe('expiry', function () {
    it('opt.expiry is embedded as e', async function () {
      const ts = Date.now() + 60000;
      const out = await ZEN.certify(bob.pub, 'inbox', alice, null, { expiry: ts });
      const data = await ZEN.verify(out, alice.pub);
      assert.strictEqual(data.e, ts);
    });
    it('opt.expiry as string is parsed to float', async function () {
      const ts = Date.now() + 60000;
      const out = await ZEN.certify(bob.pub, 'inbox', alice, null, { expiry: String(ts) });
      const data = await ZEN.verify(out, alice.pub);
      assert.strictEqual(data.e, parseFloat(String(ts)));
    });
  });

  describe('block lists', function () {
    it('opt.block with write block string', async function () {
      const out = await ZEN.certify(bob.pub, 'inbox', alice, null, { block: 'blocklist' });
      const data = await ZEN.verify(out, alice.pub);
      assert.strictEqual(data.wb, 'blocklist');
    });
    it('opt.block with .read block', async function () {
      const out = await ZEN.certify(bob.pub, 'inbox', alice, null, { block: { read: 'readblock' } });
      const data = await ZEN.verify(out, alice.pub);
      assert.strictEqual(data.rb, 'readblock');
    });
    it('opt.block with .write block soul ref', async function () {
      const ref = { '#': 'myBlockList' };
      const out = await ZEN.certify(bob.pub, 'inbox', alice, null, { block: { write: ref } });
      const data = await ZEN.verify(out, alice.pub);
      assert.deepStrictEqual(data.wb, ref);
    });
    it('opt.blacklist alias works', async function () {
      const out = await ZEN.certify(bob.pub, 'inbox', alice, null, { blacklist: 'blist' });
      const data = await ZEN.verify(out, alice.pub);
      assert.strictEqual(data.wb, 'blist');
    });
    it('opt.ban alias works', async function () {
      const out = await ZEN.certify(bob.pub, 'inbox', alice, null, { ban: 'banlist' });
      const data = await ZEN.verify(out, alice.pub);
      assert.strictEqual(data.wb, 'banlist');
    });
  });

  describe('opt.raw', function () {
    it('cert is always a compact string', async function () {
      const cert = await ZEN.certify(bob.pub, 'inbox', alice, null, { raw: 1 });
      assert.ok(isCompactSig(cert));
    });
  });

  describe('callback', function () {
    it('calls cb with compact signed string', function (done) {
      ZEN.certify(bob.pub, 'inbox', alice, function (out) {
        assert.ok(isCompactSig(out));
        done();
      });
    });
  });

  describe('error cases', function () {
    it('null certificants returns undefined', async function () {
      const out = await ZEN.certify(null, 'inbox', alice);
      assert.strictEqual(out, undefined);
    });
    it('empty object certificants returns undefined', async function () {
      const out = await ZEN.certify({}, 'inbox', alice);
      assert.strictEqual(out, undefined);
    });
    it('no policy returns undefined', async function () {
      const out = await ZEN.certify(bob.pub, {}, alice);
      assert.strictEqual(out, undefined);
    });
  });

  describe('multi-curve authority (P-256)', function () {
    it('p256 authority signs a certificate', async function () {
      const p256alice = await ZEN.pair(null, { curve: 'p256' });
      const out = await ZEN.certify(bob.pub, 'inbox', p256alice);
      assert.ok(isCompactSig(out));
    });
    it('p256 certificate has curve marker', async function () {
      const p256alice = await ZEN.pair(null, { curve: 'p256' });
      const out = await ZEN.certify(bob.pub, 'inbox', p256alice);
      assert.strictEqual(out[87], '/', 'p256 cert uses / curve separator');
      assert.ok(out.slice(88).startsWith('p256:'));
      const data = await ZEN.verify(out, p256alice.pub);
      assert.strictEqual(data.c, bob.pub);
      assert.strictEqual(data.w, 'inbox');
    });
    it('p256 certificate verifies with p256 authority pub', async function () {
      const p256alice = await ZEN.pair(null, { curve: 'p256' });
      const out = await ZEN.certify(bob.pub, 'inbox', p256alice);
      const verified = await ZEN.verify(out, p256alice.pub);
      assert.ok(verified !== undefined);
      assert.strictEqual(verified.c, bob.pub);
    });
  });

  describe('ZEN instance method', function () {
    it('zen.certify() mirrors static certify()', async function () {
      const zen = new ZEN({ localStorage: false });
      const out = await zen.certify(bob.pub, 'inbox', alice);
      assert.ok(isCompactSig(out));
    });
  });
});
