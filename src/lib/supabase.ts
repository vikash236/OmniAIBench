import { createClient } from '@supabase/supabase-js';

// Supabase configuration loaded from environment variables
const supabaseUrl = import.meta.env.VITE_SUPABASE_URL;
const supabaseAnonKey = import.meta.env.VITE_SUPABASE_ANON_KEY;

export const supabase = createClient(supabaseUrl, supabaseAnonKey);

// Type definitions for database tables
export interface BenchmarkResult {
    id?: string;
    user_id?: string;
    user_email?: string;
    created_at?: string;

    // Hardware
    cpu_name: string;
    gpu_name?: string;
    npu_name?: string;
    ram_gb: number;

    // Benchmark type
    benchmark_type: 'ai' | 'cpu' | 'gpu_compute' | 'stress' | 'quick';

    // AI results
    model_name?: string;
    framework?: 'onnx' | 'pytorch' | 'tensorflow' | 'openvino';
    provider?: string;
    latency_ms?: number;
    ips?: number;
    omniscore?: number;

    // CPU results
    single_core_score?: number;
    multi_core_score?: number;

    // GPU results
    gpu_compute_score?: number;

    // Overall
    overall_score?: number;
    app_version?: string;
}

// Helper function to save benchmark results
export async function saveBenchmarkResult(result: BenchmarkResult) {
    const { data, error } = await supabase
        .from('benchmark_results')
        .insert([result])
        .select();

    if (error) {
        console.error('Error saving benchmark result:', error);
        throw error;
    }

    return data;
}

// Helper function to get leaderboard
export async function getLeaderboard(benchmarkType?: string, limit: number = 100) {
    let query = supabase
        .from('benchmark_results')
        .select('*')
        .order('overall_score', { ascending: false })
        .limit(limit);

    if (benchmarkType) {
        query = query.eq('benchmark_type', benchmarkType);
    }

    const { data, error } = await query;

    if (error) {
        console.error('Error fetching leaderboard:', error);
        throw error;
    }

    return data;
}
