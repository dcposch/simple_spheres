import Graphics.UI.SDL as SDL
import System.Exit
import System.Random
import Data.Word

data Sphere = Sphere {
    center :: F3,
    radius :: Float }

data Graphics = Graphics {
    objs :: [Sphere],
    lights :: [F3],
    eye :: F3}

data Fragment = Fragment {
    depth :: Float,
    intersection :: F3,
    normal :: F3 }

instance Ord Fragment where
    (<=) (Fragment d i n) (Fragment d2 i2 n2) = (d <= d2)

instance Eq Fragment where
    (==) (Fragment d i n) (Fragment d2 i2 n2) = (d == d2)

data F3 = F3 {
    x :: Float,
    y :: Float,
    z :: Float }

instance Show F3 where
    show (F3 x y z) = "("++(show x)++","++(show y)++","++(show z)++")"

instance Eq F3 where
    (==) (F3 x1 y1 z1) (F3 x2 y2 z2) = (x1 == x2) && (y1 == y2) && (z1 == z2)

instance Num F3 where
    (+) (F3 x1 y1 z1) (F3 x2 y2 z2) = (F3 (x1 + x2) (y1 + y2) (z1 + z2))
    (-) (F3 x1 y1 z1) (F3 x2 y2 z2) = (F3 (x1 - x2) (y1 - y2) (z1 - z2))
    (*) (F3 x1 y1 z1) (F3 x2 y2 z2) = (F3 (x1 * x2) (y1 * y2) (z1 * z2))
    signum (F3 x1 y1 z1) = 0
    abs (F3 x y z) = (F3 0 0 0)
    fromInteger n = (F3 (fromIntegral n) (fromIntegral n) (fromIntegral n))

data Ray = Ray {
    start::F3,
    direction::F3}


width, height :: Int
width = 320
height = 240

main = withInit [InitVideo] $ 
    do screen <- setVideoMode 640 480 16 [SWSurface]
       setCaption "simple raytracer demo - haskell" ""
       enableUnicode True
       uiLoop display 0

foreach :: [a] -> (a -> IO ()) -> IO ()
foreach [] _ = return ()
foreach (x:xs) f = (f x) >> (foreach xs f)

render graphics = [[let
    ray = castRay graphics x y
    light = trace graphics ray 1
    in expose light 1 
    | x <- [0..(width-1)]] 
    | y <- [0..(height-1)]]

display :: Int -> IO ()
display i
    = do screen <- getVideoSurface
         let format = surfaceGetPixelFormat screen
         let graphics = (Graphics [
                (Sphere (F3 1 1 1) 0.3),
                (Sphere (F3 1 1 (-1)) 0.3),
                (Sphere (F3 1 (-1) 1) 0.3),
                (Sphere (F3 1 (-1) (-1)) 0.3),
                (Sphere (F3 (-1) 1 1) 0.3),
                (Sphere (F3 (-1) 1 (-1)) 0.3),
                (Sphere (F3 (-1) (-1) 1) 0.3),
                (Sphere (F3 (-1) (-1) (-1)) 0.3)] 
                []
                (F3 (sin (fromIntegral(i)/20)) 0 (-4 - (cos (fromIntegral(i)/20)))))
         
         --the entire rendering is a pure function...
         let colorBuffer = render graphics

         --only copying the rendered image out onto the screen requires the io monad
         foreach [0..(width-1)] (\x -> do
             foreach [0..(height-1)] (\y -> do
                 let format = surfaceGetPixelFormat screen
                 color <- let
                     (F3 lx ly lz) = (colorBuffer !! y) !! x 
                     in
                     mapRGB format (floor (255.9*lx)) (floor (ly*255.9)) (floor (lz*255.9))
                 result <- fillRect screen (Just (Rect x y 2 2)) color
                 return ()
                 )
             )
         putStrLn ""
         putStrLn $ "rendered frame "++(show i)

         SDL.flip screen


scale (F3 x y z) b = (F3 (x*b) (y*b) (z*b))

expose :: F3 -> Float -> F3
expose (F3 x y z) f = (F3 (1-(exp (-x * f))) (1-(exp (-y * f))) (1-(exp (-z * f))))

intersect :: Sphere -> Ray -> Fragment
intersect (Sphere center rad) (Ray start dir) =
    let
        dst = (center - start)
        t = dot dst dir
        d = t*t - (dot dst dst) + (rad*rad)
        t2 = t - (sqrt d)
        inter = start + (scale dir t2)
        normal = scale (inter - center) (1.0/rad)
    in (Fragment t2 inter normal)

intersects ss ray =
    let
        frags = map (\obj -> intersect obj ray) ss
        filterFn (Fragment d i n) = not (isNaN d) && d > 0
    in filter filterFn frags

castRay :: Graphics -> Int -> Int -> Ray
castRay graphics i j = 
    let
    normX = (fromIntegral(i)/fromIntegral(width)) - 0.5
    normY = 0.5 - (fromIntegral(j)/fromIntegral(height))
    in Ray (eye graphics) (F3 normX normY (sqrt (1.0 - (normX*normX) - (normY*normY))))

trace :: Graphics -> Ray -> Float -> F3
trace graphics (Ray start dir) depth
    = let
        fragments = intersects (objs graphics) (Ray start dir)
        frag = minimum fragments
        bounce = scale (normal frag) (dot (normal frag) dir)
        newRay = Ray (intersection frag) (dir-(2*bounce))
        cinc = dot (direction newRay) (F3 (-1) 1 (-1))  / (sqrt 3)
        highlightThresh = 0.8
        highlightMult = ((cinc - highlightThresh) / (1 - highlightThresh))**2
        highlight = if cinc > highlightThresh then (scale (F3 1 1 1) highlightMult) else (F3 0 0 0)
        specular = if depth > 0 then highlight + (trace graphics newRay (depth - 1)) else highlight
        diffuse = shade frag
      in if (length fragments) == 0 then (F3 0 0 0) else diffuse+specular

dot :: F3 -> F3 -> Float
dot a b =
    (x a)*(x b) + (y a)*(y b) + (z a)*(z b)

shade :: Fragment -> F3
shade frag
    = let
        cIncident = (dot (normal frag) (F3 (-1) 1 (-1)))/sqrt(3)
        diffuse = if cIncident > 0  then F3 (cIncident*0.8) (cIncident*0.8) (cIncident*0.2) else (F3 0 0 0)
      in diffuse

uiLoop :: (Int -> IO ()) -> Int -> IO ()
uiLoop fn i
    = do event <- pollEvent
         case event of
           Quit -> exitWith ExitSuccess
           KeyDown (Keysym _ _ 'q') -> exitWith ExitSuccess
           NoEvent -> fn i
           _ -> return ()
         uiLoop fn (i+1)

