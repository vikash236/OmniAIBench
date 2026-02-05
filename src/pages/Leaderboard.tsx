import { useState } from 'react';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Trophy, Cpu, Gpu, Brain, Download } from 'lucide-react';

interface BenchmarkScore {
    id: string;
    date: string;
    cpuSingleCore: number;
    cpuMultiCore: number;
    gpuGraphics: number;
    gpuCompute: number;
    npuInference?: number;
}

export default function Leaderboard() {
    const [scores] = useState<BenchmarkScore[]>([
        {
            id: '1',
            date: new Date().toISOString(),
            cpuSingleCore: 1245,
            cpuMultiCore: 7832,
            gpuGraphics: 8945,
            gpuCompute: 6234,
            npuInference: 956
        },
        // Add more mock data as needed
    ]);

    const [filter, setFilter] = useState<'all' | 'cpu' | 'gpu' | 'npu'>('all');

    const exportResults = () => {
        const dataStr = JSON.stringify(scores, null, 2);
        const dataBlob = new Blob([dataStr], { type: 'application/json' });
        const url = URL.createObjectURL(dataBlob);
        const link = document.createElement('a');
        link.href = url;
        link.download = 'benchmark-results.json';
        link.click();
    };

    return (
        <div className="space-y-4">
            {/* Header */}
            <div className="pb-3 border-b border-border flex items-center justify-between">
                <div>
                    <h1 className="text-base font-semibold">Leaderboard</h1>
                    <p className="text-xs text-muted-foreground mt-0.5">
                        Your benchmark history and scores
                    </p>
                </div>
                <Button onClick={exportResults} variant="outline" size="sm" className="text-xs h-7">
                    <Download className="h-3 w-3 mr-1.5" />
                    Export
                </Button>
            </div>

            {/* Filter Buttons */}
            <div className="flex gap-2">
                <Button
                    onClick={() => setFilter('all')}
                    variant={filter === 'all' ? 'default' : 'outline'}
                    size="sm"
                    className="text-xs h-7"
                >
                    All Tests
                </Button>
                <Button
                    onClick={() => setFilter('cpu')}
                    variant={filter === 'cpu' ? 'default' : 'outline'}
                    size="sm"
                    className="text-xs h-7"
                >
                    <Cpu className="h-3 w-3 mr-1.5" />
                    CPU
                </Button>
                <Button
                    onClick={() => setFilter('gpu')}
                    variant={filter === 'gpu' ? 'default' : 'outline'}
                    size="sm"
                    className="text-xs h-7"
                >
                    <Gpu className="h-3 w-3 mr-1.5" />
                    GPU
                </Button>
                <Button
                    onClick={() => setFilter('npu')}
                    variant={filter === 'npu' ? 'default' : 'outline'}
                    size="sm"
                    className="text-xs h-7"
                >
                    <Brain className="h-3 w-3 mr-1.5" />
                    NPU
                </Button>
            </div>

            {/* Scores Table */}
            <Card className="border border-border bg-card">
                <CardHeader className="pb-3">
                    <CardTitle className="flex items-center gap-2 text-sm">
                        <Trophy className="h-4 w-4 text-warning" />
                        Benchmark Results
                    </CardTitle>
                </CardHeader>
                <CardContent>
                    {scores.length === 0 ? (
                        <div className="text-center py-12">
                            <Trophy className="h-12 w-12 mx-auto text-muted-foreground/30 mb-3" />
                            <p className="text-sm text-muted-foreground">No benchmark results yet</p>
                            <p className="text-xs text-muted-foreground mt-1">
                                Run benchmarks to see your scores here
                            </p>
                        </div>
                    ) : (
                        <div className="overflow-x-auto">
                            <table className="w-full text-xs">
                                <thead>
                                    <tr className="border-b border-border">
                                        <th className="text-left py-2 px-2 font-medium text-muted-foreground">Date</th>
                                        <th className="text-left py-2 px-2 font-medium text-muted-foreground">CPU Single</th>
                                        <th className="text-left py-2 px-2 font-medium text-muted-foreground">CPU Multi</th>
                                        <th className="text-left py-2 px-2 font-medium text-muted-foreground">GPU Graphics</th>
                                        <th className="text-left py-2 px-2 font-medium text-muted-foreground">GPU Compute</th>
                                        <th className="text-left py-2 px-2 font-medium text-muted-foreground">NPU</th>
                                    </tr>
                                </thead>
                                <tbody>
                                    {scores.map((score, index) => (
                                        <tr key={score.id} className="border-b border-border/50 hover:bg-muted/30">
                                            <td className="py-2 px-2">
                                                {index === 0 && <Trophy className="h-3 w-3 inline mr-1 text-warning" />}
                                                {new Date(score.date).toLocaleDateString()}
                                            </td>
                                            <td className="py-2 px-2 font-mono text-primary">{score.cpuSingleCore}</td>
                                            <td className="py-2 px-2 font-mono text-secondary">{score.cpuMultiCore}</td>
                                            <td className="py-2 px-2 font-mono text-accent">{score.gpuGraphics}</td>
                                            <td className="py-2 px-2 font-mono text-info">{score.gpuCompute}</td>
                                            <td className="py-2 px-2 font-mono text-warning">
                                                {score.npuInference || 'N/A'}
                                            </td>
                                        </tr>
                                    ))}
                                </tbody>
                            </table>
                        </div>
                    )}
                </CardContent>
            </Card>

            {/* Stats Summary */}
            {scores.length > 0 && (
                <div className="grid grid-cols-1 md:grid-cols-3 gap-3">
                    <Card className="border border-border bg-card">
                        <CardHeader className="pb-2">
                            <CardTitle className="text-xs font-medium text-muted-foreground">
                                Total Tests Run
                            </CardTitle>
                        </CardHeader>
                        <CardContent>
                            <div className="text-2xl font-bold font-mono">{scores.length}</div>
                        </CardContent>
                    </Card>

                    <Card className="border border-border bg-card">
                        <CardHeader className="pb-2">
                            <CardTitle className="text-xs font-medium text-muted-foreground">
                                Best CPU Score
                            </CardTitle>
                        </CardHeader>
                        <CardContent>
                            <div className="text-2xl font-bold text-primary font-mono">
                                {Math.max(...scores.map(s => s.cpuMultiCore))}
                            </div>
                        </CardContent>
                    </Card>

                    <Card className="border border-border bg-card">
                        <CardHeader className="pb-2">
                            <CardTitle className="text-xs font-medium text-muted-foreground">
                                Best GPU Score
                            </CardTitle>
                        </CardHeader>
                        <CardContent>
                            <div className="text-2xl font-bold text-accent font-mono">
                                {Math.max(...scores.map(s => s.gpuGraphics))}
                            </div>
                        </CardContent>
                    </Card>
                </div>
            )}
        </div>
    );
}
