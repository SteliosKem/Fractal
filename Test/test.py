# compare_greedy_vs_global_numpy.py
import numpy as np, math, random, time
np.random.seed(1); random.seed(1)

d = 1.0
tol = 1e-6

def pairwise_S(points):
    m = points.shape[0]; S = 0.0
    for i in range(m):
        for j in range(i+1, m):
            diff = points[i] - points[j]; S += diff.dot(diff)
    return S

def min_pairwise(points):
    m = points.shape[0]; minr = 1e9
    for i in range(m):
        for j in range(i+1, m):
            dist = np.linalg.norm(points[i]-points[j])
            if dist < minr: minr = dist
    return minr

def greedy_arrangement(n, fixed_points, samples=2000, refine_steps=400):
    pts = fixed_points.copy().tolist()
    for k in range(len(fixed_points)+1, n+1):
        existing = np.array(pts); centroid = existing.mean(axis=0)
        best_val = 1e18; best_x = None
        for s in range(samples):
            r = abs(np.random.randn())*1.0 + 0.2*np.random.rand()
            theta = np.random.rand()*2*math.pi
            cand = centroid + np.array([r*math.cos(theta), r*math.sin(theta)])
            if np.any(np.linalg.norm(existing - cand, axis=1) < d): continue
            val = np.sum(np.sum((existing - cand)**2, axis=1))
            if val < best_val: best_val = val; best_x = cand.copy()
        if best_x is None: best_x = centroid + np.array([d,0.0])
        pts.append(best_x)
        # refine by small random moves
        for it in range(refine_steps):
            cur = pts[-1]; cand = cur + 0.06*np.random.randn(2)
            existing = np.array(pts[:-1])
            if np.any(np.linalg.norm(existing - cand, axis=1) < d): continue
            val_cur = np.sum(np.sum((existing - cur)**2, axis=1))
            val_cand = np.sum(np.sum((existing - cand)**2, axis=1))
            if val_cand < val_cur: pts[-1] = cand
    return np.array(pts)

def random_feasible(n, fixed_points, max_attempts=5000):
    k = fixed_points.shape[0]; pts = np.zeros((n,2)); pts[:k,:] = fixed_points
    m = n - k; centroid = fixed_points.mean(axis=0); R = 2.0 + 0.6*math.sqrt(n)
    placed = k; attempts = 0
    while placed < n and attempts < max_attempts:
        cand = centroid + (np.random.rand(2)*2-1)*R
        if np.all(np.linalg.norm(pts[:placed] - cand, axis=1) >= d): pts[placed] = cand; placed += 1
        attempts += 1
    return (pts if placed==n else None)

def hillclimb_local(pts, iterations=2000, step_scale=0.5):
    best = pts.copy(); bestS = pairwise_S(best); n = pts.shape[0]
    for it in range(iterations):
        i = np.random.randint(2, n)
        cur = best[i].copy(); step = step_scale*(0.2+0.8*np.random.rand())*np.random.randn(2)
        cand = cur + step; others = np.concatenate([best[:i], best[i+1:]])
        if np.any(np.linalg.norm(others - cand, axis=1) < d): continue
        new = best.copy(); new[i] = cand
        newS = pairwise_S(new)
        if newS < bestS: best = new; bestS = newS
    return best, bestS

def find_best_global(n, fixed_points, restarts=200):
    best_overall = None; bestS = 1e18
    for r in range(restarts):
        pts = random_feasible(n, fixed_points)
        if pts is None: continue
        pts, S0 = hillclimb_local(pts, iterations=1500, step_scale=0.6)
        if S0 < bestS: bestS = S0; best_overall = pts.copy()
    return best_overall, bestS

def run_exp(nmax=1000):
    fixed = np.array([[0.,0.],[1.,0.]])
    for n in range(3, nmax+1):
        print("n=", n)
        gpts = greedy_arrangement(n, fixed)
        gS = pairwise_S(gpts); gmin = min_pairwise(gpts)
        bpts, bS = find_best_global(n, fixed, restarts=400)
        feasible = (bpts is not None and min_pairwise(bpts) >= d - 1e-6)
        print(" greedy S=%.6f min_pairwise=%.6f ; best_found S=%s feasible=%s" % (gS, gmin, ("%.6f" % bS) if feasible else None, feasible))
    print("done")

if __name__=="__main__":
    run_exp(1000)
