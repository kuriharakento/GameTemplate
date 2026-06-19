#pragma once

#include <vector>
#include <optional>
#include <functional>
#include <string>
#include <memory>


 /**
  * @enum BuffType
  * @brief バフの適用方法を定義
  */
enum class BuffType
{
    Additive,        // 加算バフ（+10など）
    Multiplicative,  // 乗算バフ（×1.5など）
    Percentage,      // 割合バフ（+50%など）
    Override         // 上書きバフ（石化で移動速度を0にするなど）
};

/**
 * @enum BuffPriority
 * @brief バフの計算優先度
 */
enum class BuffPriority
{
    Low = 0,      // 低優先度: デバフや弱体化効果
    Normal = 1,   // 通常優先度: 装備品や基本的なバフ
    High = 2      // 高優先度: 一時的な強化スキルやアイテム効果
};

/**
 * @struct BuffConfig
 * @brief バフの設定情報を定義する構造体
 */
struct BuffConfig
{
    // バフの一意識別子（例: "fire_buff", "speed_boost"）
    std::string id;
    // バフの効果値（種類により意味が異なる）
    float value;
    // バフの種類（加算、乗算、割合、上書き）
    BuffType type;
    // 持続時間（秒）。nulloptの場合は永続バフ
    std::optional<float> duration;
    // 計算優先度（デフォルト: Normal）
    BuffPriority priority;
    // 最大スタック数（0=無制限、デフォルト: 1）
    int maxStacks;
    // 同じバフを再度付与した時に持続時間をリセットするか
    bool refreshable;
    // バフのタグ（カテゴリ分類用）
    std::vector<std::string> tags;

    /**
     * @brief BuffConfigのコンストラクタ
     *
     * @param id バフの一意識別子
     * @param val バフの効果値
     * @param t バフの種類
     * @param dur 持続時間（秒）。nulloptで永続バフ
     */
    BuffConfig(const std::string& id, float val, BuffType t, std::optional<float> dur = std::nullopt);

    /**
     * @brief タグを追加
     * @param tag 追加するタグ
     * @return 自身の参照
     */
    BuffConfig& AddTag(const std::string& tag);

    /**
     * @brief 特定のタグを持っているか確認
     * @param tag 確認するタグ
     * @return タグを持っていればtrue
     */
    bool HasTag(const std::string& tag) const;
};
/**
 * @class BuffInstance
 * @brief 実際にステータスに適用されているバフのインスタンス
 */
class BuffInstance
{
public:
    /**
     * @brief BuffInstanceのコンストラクタ
     * @param config バフの設定情報
     */
    explicit BuffInstance(const BuffConfig& config);

    // ===============================================
    // Getter メソッド群
    // ===============================================

    /**
     * @brief バフのIDを取得
     * @return バフの一意識別子
     */
    const std::string& GetId() const;

    /**
     * @brief バフの現在の効果値を取得
     * 
     * @return 効果値 × スタック数
     * @note スタック数が3、効果値が5なら、15を返します
     */
    float GetValue() const;

    /**
     * @brief バフの種類を取得
     * @return BuffType (Additive, Multiplicative, Percentage, Override)
     */
    BuffType GetType() const;

    /**
     * @brief バフの優先度を取得
     * @return BuffPriority (Low, Normal, High)
     */
    BuffPriority GetPriority() const;

    /**
     * @brief 現在のスタック数を取得
     * @return スタック数（1以上）
     */
    int GetStackCount() const;

    /**
     * @brief バフが期限切れかどうかを判定
     * @return 期限切れならtrue、まだ有効ならfalse
     */
    bool IsExpired() const;

    /**
     * @brief 永続バフかどうかを判定
     * @return 永続バフならtrue、時限バフならfalse
     */
    bool IsPermanent() const;

    /**
     * @brief 残り時間を取得
     * @return 残り時間（秒）。永続バフの場合は-1.0f
     */
    float GetRemainingTime() const;

    /**
     * @brief 特定のタグを持っているか確認
     * @param tag 確認するタグ
     * @return タグを持っていればtrue
     */
    bool HasTag(const std::string& tag) const;

    /**
     * @brief 全てのタグを取得
     * @return タグのvector
     */
    const std::vector<std::string>& GetTags() const;

    /**
     * @brief バフの残り時間を更新
     * @param deltaTime 経過時間（秒）
     */
    void Update(float deltaTime);

    /**
     * @brief スタック数を1増やす
     * @return 成功したらtrue、スタック上限に達していたらfalse
     */
    bool TryAddStack();

    /**
     * @brief バフの持続時間をリセット
     */
    void RefreshDuration();

private:
    // バフの設定情報
    BuffConfig config_;
    // 残り時間（永続バフの場合はnullopt）
    std::optional<float> remainingTime_;
    // 現在のスタック数
    int stackCount_;
};

/**
 * @struct StatusChangeEvent
 * @brief ステータス値が変更された時のイベント情報
 */
struct StatusChangeEvent
{
    // 変更前の値
    float oldValue;
    // 変更後の値
    float newValue;
    // 変更の原因となったバフID（複数の場合は空文字列）
    std::string causeBuffId;
};

/**
 * @brief ステータス値とバフシステムを管理するクラス
 * 
 * RPGなどのゲームでよく使われる、ステータス値（HP、攻撃力など）と
 * それに適用されるバフ・デバフを管理します。
 * 
 * バフの種類:
 * - Additive: 加算バフ（+10など）
 * - Multiplicative: 乗算バフ（×1.5など）
 * - Percentage: 割合バフ（+50%など）
 * - Override: 上書きバフ（石化で移動速度を0にするなど）
 * 
 * @note キャッシュシステムにより、バフが変更されない限り計算結果を再利用します
 */
class StatusValue
{
public:
    // ステータス変更時のコールバック関数型
    using ChangeCallback = std::function<void(const StatusChangeEvent&)>;

    explicit StatusValue(float baseValue = 0.0f);
    virtual ~StatusValue() = default;

    // ムーブを許可
    StatusValue(StatusValue&&) noexcept = default;
    StatusValue& operator=(StatusValue&&) noexcept = default;

    // コピーは禁止（unique_ptrが含まれるため）
    StatusValue(const StatusValue&) = delete;
    StatusValue& operator=(const StatusValue&) = delete;

    // ===============================================
    // 基礎値の取得・設定
    // ===============================================

    /**
     * @brief 基礎値を取得
     * @return 現在の基礎値
     */
    float GetBase() const;

    /**
     * @brief 基礎値を設定
     * @param value 設定する基礎値
     */
    void SetBase(float value);

    /**
     * @brief 最終的なステータス値を取得
     * 
     * キャッシュが有効な場合は即座に返し、無効な場合は再計算します。
     * 
     * @return 基礎値 + 全てのバフを適用した最終値
     */
    float GetValue() const;

    // ===============================================
    // バフの管理
    // ===============================================

    /**
     * @brief 新しいバフを追加
     * 
     * 同じIDのバフが既に存在する場合、スタック数を増やすか、
     * 持続時間をリフレッシュします（設定による）。
     * 
     * @param config バフの設定情報
     * @return 追加成功でtrue、スタック上限で失敗した場合false
     */
    bool AddBuff(const BuffConfig& config);

    /**
     * @brief 特定のバフを削除
     * @param buffId 削除するバフのID
     * @return 削除成功でtrue、バフが見つからなかった場合false
     */
    bool RemoveBuff(const std::string& buffId);

    /**
     * @brief 全てのバフを削除
     */
    void ClearAllBuffs();

    /**
     * @brief 特定の種類のバフを全て削除
     * @param type 削除するバフの種類
     * @return 削除したバフの数
     */
    int ClearBuffsByType(BuffType type);

    /**
     * @brief 条件に合致するバフを削除
     * @param predicate 削除条件を判定する関数
     * @return 削除したバフの数
     */
    int RemoveBuffIf(std::function<bool(const BuffInstance*)> predicate);

    /**
     * @brief 特定のタグを持つバフを全て削除
     * @param tag 削除するバフのタグ
     * @return 削除したバフの数
     */
    int RemoveBuffsByTag(const std::string& tag);

    /**
     * @brief 特定の優先度のバフを全て削除
     * @param priority 削除するバフの優先度
     * @return 削除したバフの数
     */
    int RemoveBuffsByPriority(BuffPriority priority);

    /**
     * @brief バフの更新処理
     * @param deltaTime 経過時間（秒）
     */
    void Update(float deltaTime);

    /**
     * @brief バフの存在確認
     * @param buffId 確認するバフのID
     * @return バフが存在すればtrue
     */
	bool HasBuff(const std::string& buffId) const;

    /**
     * @brief 特定のバフの残り時間を取得
     * @param buffId バフのID
     * @return 残り時間（永続バフの場合はnullopt）
     */
	std::optional<float> GetBuffRemainingTime(const std::string& buffId) const;

    /**
     * @brief 全てのバフIDを取得
     * @return バフIDのvector
     */
    std::vector<std::string> GetBuffIds() const;

    /**
     * @brief 適用中のバフ数を取得
     * @return バフの数
     */
    size_t GetBuffCount() const;

    /**
     * @brief ステータス変更時のコールバックを設定
     * @param callback ステータス変更時に呼び出される関数
     */
    void SetChangeCallback(ChangeCallback callback);

private: // メンバ関数
    /**
     * @brief キャッシュを無効化
     */
    void MarkDirty();

    /**
     * @brief キャッシュが無効な場合に値を再計算
     * @return 計算されたステータス値
     */
    float CalculateValue() const;

private: // メンバ変数
    // 基礎値
    float base_;
    // キャッシュされた計算結果（const関数内で更新するためmutable）
    mutable float cachedValue_;
    // キャッシュが無効かどうか（const関数内で更新するためmutable）
    mutable bool isDirty_;
    // 適用中のバフリスト
    std::vector<std::unique_ptr<BuffInstance>> buffs_;
    // 値変更時のコールバック
    ChangeCallback changeCallback_;
};