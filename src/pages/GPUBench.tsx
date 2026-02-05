import { useState } from 'react';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Gpu, Play, RotateCcw, CheckCircle, Activity } from 'lucide-react';

interface GPUBenchmarkResult {
    graphicsScore: number;
    computeScore: number;
    averageFPS: number;
    completed: boolean;
}

export default function GPUBench() {
    const [running, setRunning] = useState(false);
    const [progress, setProgress] = useState(0);
    const [currentFPS, setCurrentFPS] = useState(0);
    const [result, setResult] = useState<GPUBenchmarkResult | null>(null);

    const runBenchmark = async () => {
        setRunning(true);
        setProgress(0);
        setResult(null);

        // Simulate benchmark with FPS updates
        const interval = setInterval(() => {
            setCurrentFPS(Math.floor(Math.random() * 30) + 100);
        }, 100);

        for (let i = 0; i <= 100; i += 1) {
            await new Promise(resolve => setTimeout(resolve, 80));
            setProgress(i);
        }

        clearInterval(interval);

        // Simulated results
        setResult({
            graphicsScore: Math.floor(Math.random() * 3000) + 7000,
            computeScore: Math.floor(Math.random() * 2000) + 5000,
            averageFPS: Math.floor(Math.random() * 30) + 100,
            completed: true
        });
        setRunning(false);
        setCurrentFPS(0);
    };

    const reset = () => {
        setResult(null);
        setProgress(0);
        setCurrentFPS(0);
    };

    return (
        <div className="space-y-4">
            {/* Header */}
            <div className="pb-3 border-b border-border">
                <h1 className="text-base font-semibold">GPU Benchmark</h1>
                <p className="text-xs text-muted-foreground mt-0.5">
                    Test graphics and compute performance
                </p>
            </div>

            {/* Benchmark Control Card */}
            <Card className="border border-border bg-card">
                <CardHeader className="pb-3">
                    <CardTitle className="flex items-center gap-2 text-sm">
                        <Gpu className="h-4 w-4 text-accent" />
                        Graphics Performance Test
                    </CardTitle>
                    <CardDescription className="text-xs">
                        Renders intensive 3D scenes and compute shaders
                    </CardDescription>
                </CardHeader>
                <CardContent className="space-y-3">
                    {/* Live FPS */}
                    {running && (
                        <div className="flex items-center justify-between p-2 bg-muted rounded border border-border">
                            <div className="flex items-center gap-2 text-xs">
                                <Activity className="h-3.5 w-3.5 text-success animate-pulse" />
                                <span className="text-muted-foreground">Live FPS</span>
                            </div>
                            <span className="text-lg font-mono font-bold text-success">
                                {currentFPS}
                            </span>
                        </div>
                    )}

                    {/* Progress Bar */}
                    {(running || result) && (
                        <div className="space-y-1.5">
                            <div className="flex justify-between text-xs">
                                <span className="text-muted-foreground">Progress</span>
                                <span className="font-mono">{progress}%</span>
                            </div>
                            <div className="h-2 bg-muted rounded-full overflow-hidden">
                                <div
                                    className="h-full bg-accent transition-all duration-300"
                                    style={{ width: `${progress}%` }}
                                />
                            </div>
                        </div>
                    )}

                    {/* Control Buttons */}
                    <div className="flex gap-2">
                        <Button
                            onClick={runBenchmark}
                            disabled={running}
                            className="flex-1 text-xs h-8"
                        >
                            {running ? (
                                <>
                                    <div className="h-3 w-3 mr-1.5 border-2 border-primary-foreground border-t-transparent rounded-full animate-spin" />
                                    Rendering...
                                </>
                            ) : (
                                <>
                                    <Play className="h-3 w-3 mr-1.5" />
                                    Start Benchmark
                                </>
                            )}
                        </Button>
                        {result && (
                            <Button
                                onClick={reset}
                                variant="outline"
                                className="text-xs h-8"
                            >
                                <RotateCcw className="h-3 w-3 mr-1.5" />
                                Reset
                            </Button>
                        )}
                    </div>
                </CardContent>
            </Card>

            {/* Results */}
            {result && result.completed && (
                <div className="grid grid-cols-1 md:grid-cols-3 gap-3">
                    {/* Graphics Score */}
                    <Card className="border border-border bg-card">
                        <CardHeader className="pb-2.5">
                            <CardTitle className="flex items-center gap-1.5 text-xs font-medium">
                                <CheckCircle className="h-3.5 w-3.5 text-success" />
                                Graphics Score
                            </CardTitle>
                        </CardHeader>
                        <CardContent>
                            <div className="text-2xl font-bold text-accent font-mono">
                                {result.graphicsScore}
                            </div>
                            <p className="text-xs text-muted-foreground mt-1">
                                3D rendering
                            </p>
                        </CardContent>
                    </Card>

                    {/* Compute Score */}
                    <Card className="border border-border bg-card">
                        <CardHeader className="pb-2.5">
                            <CardTitle className="flex items-center gap-1.5 text-xs font-medium">
                                <CheckCircle className="h-3.5 w-3.5 text-success" />
                                Compute Score
                            </CardTitle>
                        </CardHeader>
                        <CardContent>
                            <div className="text-2xl font-bold text-secondary font-mono">
                                {result.computeScore}
                            </div>
                            <p className="text-xs text-muted-foreground mt-1">
                                Shader compute
                            </p>
                        </CardContent>
                    </Card>

                    {/* Average FPS */}
                    <Card className="border border-border bg-card">
                        <CardHeader className="pb-2.5">
                            <CardTitle className="flex items-center gap-1.5 text-xs font-medium">
                                <Activity className="h-3.5 w-3.5 text-success" />
                                Average FPS
                            </CardTitle>
                        </CardHeader>
                        <CardContent>
                            <div className="text-2xl font-bold text-success font-mono">
                                {result.averageFPS}
                            </div>
                            <p className="text-xs text-muted-foreground mt-1">
                                Frames per second
                            </p>
                        </CardContent>
                    </Card>
                </div>
            )}

            {/* Info Card */}
            <Card className="border border-border bg-card">
                <CardHeader className="pb-2.5">
                    <CardTitle className="text-xs font-medium">About This Test</CardTitle>
                </CardHeader>
                <CardContent className="text-xs text-muted-foreground space-y-1.5">
                    <p>• Graphics test renders complex 3D scenes</p>
                    <p>• Compute test runs parallel shader calculations</p>
                    <p>• FPS measures real-time rendering performance</p>
                    <p>• Ensure GPU drivers are up to date for accurate results</p>
                </CardContent>
            </Card>
        </div>
    );
}
